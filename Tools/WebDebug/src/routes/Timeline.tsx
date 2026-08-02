// 전투 타임라인 — 하나의 시간축에 4개 레인을 세로로 쌓는다.
//   레인1 게이지(선)  레인2 어빌리티 구간(간트)  레인3 태그 구간(간트)  레인4 이벤트 마커
//
// 4500포인트를 60fps로 그려야 하므로 SVG 차트 라이브러리를 쓰지 않고 canvas 에 직접 그린다.
// 화면에 보이는 구간만 렌더하고, 픽셀당 표본이 1개를 넘으면 min/max 로 다운샘플한다.
// 이 캔버스 안에는 spring 애니메이션을 절대 넣지 않는다(15Hz 스트림과 겹치면 잔상·프레임 드랍).

import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import Legend from '../components/Legend';
import { CHROME, GAUGE_SERIES, HIT_COLOR, MARKER_STYLE, seriesColor, tagBarColor, type ThemeMode } from '../lib/palette';
import { shortTag, type GaugeKey, type MarkerKey, type TimelineEvent, type TimelineSnapshot } from '../lib/schema';
import type { FeedState, TimelineFeed } from '../lib/ws';

const FRAME = 1 / 15;
const GUTTER = 74;
const PAD_RIGHT = 14;
const AXIS_H = 22;

type LaneId = 'gauge' | 'ability' | 'tag' | 'marker';

interface Span {
	src: 'player' | 'boss';
	key: string;
	t0: number;
	t1: number;
}

interface GaugeTrack {
	src: 'player' | 'boss';
	key: GaugeKey;
	t: number[];
	norm: number[];
	raw: number[];
}

interface Model {
	gauges: Map<string, GaugeTrack>;
	abilities: Span[];
	tags: Span[];
	markers: TimelineEvent[];
	hits: TimelineEvent[];
	tagRows: string[];
	tEnd: number;
}

/** begin/end 쌍을 구간으로 접는다. 아직 end 가 오지 않은 구간은 현재 시각까지 열어 둔다. */
function buildModel(events: TimelineEvent[], latestT: number): Model {
	const gauges = new Map<string, GaugeTrack>();
	const abilities: Span[] = [];
	const tags: Span[] = [];
	const markers: TimelineEvent[] = [];
	const hits: TimelineEvent[] = [];
	const openAbility = new Map<string, number>();
	const openTag = new Map<string, number>();
	let tEnd = latestT;

	for (const e of events) {
		if (e.t > tEnd) tEnd = e.t;
		switch (e.type) {
			case 'gauge': {
				const id = `${e.src}:${e.key}`;
				let track = gauges.get(id);
				if (!track) {
					track = { src: e.src, key: e.key as GaugeKey, t: [], norm: [], raw: [] };
					gauges.set(id, track);
				}
				track.t.push(e.t);
				track.norm.push(e.norm ?? 0);
				track.raw.push(e.raw ?? 0);
				break;
			}
			case 'ability':
			case 'tag': {
				const open = e.type === 'ability' ? openAbility : openTag;
				const bucket = e.type === 'ability' ? abilities : tags;
				const id = `${e.src}:${e.key}`;
				if (e.phase === 'begin') {
					open.set(id, e.t);
				} else if (e.phase === 'end') {
					const t0 = open.get(id);
					if (t0 !== undefined) {
						open.delete(id);
						bucket.push({ src: e.src, key: e.key, t0, t1: e.t });
					}
				} else {
					bucket.push({ src: e.src, key: e.key, t0: e.t, t1: e.t + FRAME });
				}
				break;
			}
			case 'marker':
				markers.push(e);
				break;
			case 'hit':
				hits.push(e);
				break;
		}
	}

	// 아직 닫히지 않은 구간은 현재 시각까지 그린다
	for (const [id, t0] of openAbility) {
		const [src, ...rest] = id.split(':');
		abilities.push({ src: src as 'player' | 'boss', key: rest.join(':'), t0, t1: tEnd });
	}
	for (const [id, t0] of openTag) {
		const [src, ...rest] = id.split(':');
		tags.push({ src: src as 'player' | 'boss', key: rest.join(':'), t0, t1: tEnd });
	}

	// 태그 행 순서 — 처음 등장한 순서를 유지해 화면이 튀지 않게 한다
	const seen: string[] = [];
	for (const s of tags) {
		const label = `${s.src}:${s.key}`;
		if (!seen.includes(label)) seen.push(label);
	}

	return { gauges, abilities, tags, markers, hits, tagRows: seen, tEnd };
}

/** t 이하인 마지막 인덱스 (이진 탐색) */
function lowerIndex(arr: number[], t: number): number {
	let lo = 0;
	let hi = arr.length - 1;
	let ans = -1;
	while (lo <= hi) {
		const mid = (lo + hi) >> 1;
		if (arr[mid] <= t) {
			ans = mid;
			lo = mid + 1;
		} else {
			hi = mid - 1;
		}
	}
	return ans;
}

interface Props {
	feed: TimelineFeed;
	state: FeedState;
	theme: ThemeMode;
}

export default function Timeline({ feed, state, theme }: Props) {
	const wrapRef = useRef<HTMLDivElement>(null);
	const canvasRef = useRef<HTMLCanvasElement>(null);

	const [playing, setPlaying] = useState(true);
	const [playhead, setPlayhead] = useState(0);
	const [view, setView] = useState<{ t0: number; t1: number }>({ t0: 0, t1: 20 });
	const [hiddenSeries, setHiddenSeries] = useState<Set<string>>(new Set());
	const [lanes, setLanes] = useState<Record<LaneId, boolean>>({ gauge: true, ability: true, tag: true, marker: true });
	const [hover, setHover] = useState<{ x: number; y: number; t: number } | null>(null);
	const [dragOver, setDragOver] = useState(false);
	const [size, setSize] = useState({ w: 900, h: 520 });

	const model = useMemo(() => buildModel(state.events, state.latestT), [state.events, state.latestT]);
	const tEnd = Math.max(model.tEnd, 1);

	// 라이브 모드에서는 재생 헤드와 뷰가 최신 시각을 따라간다
	useEffect(() => {
		if (!playing) return;
		setPlayhead(tEnd);
		setView((v) => {
			const span = Math.max(0.5, v.t1 - v.t0);
			return { t0: Math.max(0, tEnd - span), t1: Math.max(span, tEnd) };
		});
	}, [playing, tEnd]);

	// 캔버스 크기 추적
	useEffect(() => {
		const el = wrapRef.current;
		if (!el) return;
		const ro = new ResizeObserver(() => {
			setSize({ w: el.clientWidth, h: el.clientHeight });
		});
		ro.observe(el);
		setSize({ w: el.clientWidth, h: el.clientHeight });
		return () => ro.disconnect();
	}, []);

	const clampView = useCallback(
		(t0: number, t1: number) => {
			const minSpan = 0.4;
			let span = Math.max(minSpan, Math.min(t1 - t0, Math.max(tEnd, minSpan)));
			let s = t0;
			if (s < 0) s = 0;
			if (s + span > tEnd) s = Math.max(0, tEnd - span);
			return { t0: s, t1: s + span };
		},
		[tEnd]
	);

	/** 재생 헤드가 뷰 밖으로 나가면 뷰를 따라 움직인다 */
	const moveHead = useCallback(
		(t: number) => {
			const clamped = Math.max(0, Math.min(tEnd, t));
			setPlayhead(clamped);
			setView((v) => {
				if (clamped >= v.t0 && clamped <= v.t1) return v;
				const span = v.t1 - v.t0;
				return clampView(clamped - span / 2, clamped + span / 2);
			});
		},
		[tEnd, clampView]
	);

	const xToTime = useCallback(
		(x: number) => view.t0 + ((x - GUTTER) / (size.w - GUTTER - PAD_RIGHT)) * (view.t1 - view.t0),
		[view, size.w]
	);

	/* ── 입력 ─────────────────────────────────────────────── */

	const onWheel = useCallback(
		(e: React.WheelEvent) => {
			e.preventDefault();
			const rect = canvasRef.current!.getBoundingClientRect();
			const anchor = xToTime(e.clientX - rect.left);
			const factor = e.deltaY > 0 ? 1.25 : 0.8;
			const span = (view.t1 - view.t0) * factor;
			const ratio = (anchor - view.t0) / (view.t1 - view.t0);
			setView(clampView(anchor - span * ratio, anchor - span * ratio + span));
		},
		[view, xToTime, clampView]
	);

	const scrubFromEvent = useCallback(
		(clientX: number) => {
			const rect = canvasRef.current!.getBoundingClientRect();
			moveHead(xToTime(clientX - rect.left));
		},
		[xToTime, moveHead]
	);

	const onPointerDown = useCallback(
		(e: React.PointerEvent) => {
			(e.target as HTMLElement).setPointerCapture(e.pointerId);
			setPlaying(false);
			scrubFromEvent(e.clientX);
		},
		[scrubFromEvent]
	);

	const onPointerMove = useCallback(
		(e: React.PointerEvent) => {
			const rect = canvasRef.current!.getBoundingClientRect();
			const x = e.clientX - rect.left;
			setHover({ x, y: e.clientY - rect.top, t: xToTime(x) });
			if (e.buttons & 1) scrubFromEvent(e.clientX);
		},
		[xToTime, scrubFromEvent]
	);

	const onKeyDown = useCallback(
		(e: React.KeyboardEvent) => {
			const step = e.shiftKey ? FRAME * 10 : FRAME;
			if (e.key === 'ArrowRight') {
				e.preventDefault();
				setPlaying(false);
				moveHead(playhead + step);
			} else if (e.key === 'ArrowLeft') {
				e.preventDefault();
				setPlaying(false);
				moveHead(playhead - step);
			} else if (e.key === ' ') {
				e.preventDefault();
				setPlaying((p) => !p);
			}
		},
		[playhead, moveHead]
	);

	const onDrop = useCallback(
		async (e: React.DragEvent) => {
			e.preventDefault();
			setDragOver(false);
			const file = e.dataTransfer.files?.[0];
			if (!file) return;
			try {
				const snapshot = JSON.parse(await file.text()) as TimelineSnapshot;
				if (!Array.isArray(snapshot.events)) throw new Error('events 배열이 없습니다');
				feed.loadSnapshot(snapshot);
				setPlaying(false);
				setPlayhead(0);
				const lastEvent = snapshot.events[snapshot.events.length - 1];
				setView({ t0: 0, t1: Math.min(20, Math.max(1, lastEvent?.t ?? 20)) });
			} catch (err) {
				alert(`스냅샷을 읽지 못했습니다: ${err}`);
			}
		},
		[feed]
	);

	/* ── 렌더 ─────────────────────────────────────────────── */

	useEffect(() => {
		const canvas = canvasRef.current;
		if (!canvas || size.w <= 0 || size.h <= 0) return;
		const dpr = Math.min(2, window.devicePixelRatio || 1);
		canvas.width = Math.round(size.w * dpr);
		canvas.height = Math.round(size.h * dpr);
		const ctx = canvas.getContext('2d')!;
		ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
		draw(ctx, {
			w: size.w,
			h: size.h,
			view,
			model,
			theme,
			lanes,
			hiddenSeries,
			playhead,
			hoverT: hover?.t ?? null,
		});
	}, [size, view, model, theme, lanes, hiddenSeries, playhead, hover]);

	const session = state.session;
	const tooltip = hover ? buildTooltip(model, hover.t, hiddenSeries) : null;

	return (
		<div className="timeline">
			<div className="sessionbar">
				{session ? (
					<>
						<span>
							시드 <b>{session.runSeed}</b>
						</span>
						<span>
							보스 <b>{shortTag(session.bossId)}</b>
						</span>
						<span>
							시도 <b>{session.attempt}회차</b>
						</span>
						<span>
							무기 <b>{shortTag(session.weapon)}</b>
						</span>
						<span>
							빌드 <b>{session.buildConfig}</b>
						</span>
						<span>
							경과 <b>{tEnd.toFixed(1)}s</b>
						</span>
						<span className="spacer" />
						<span className="hint">세션 {session.sessionId}</span>
					</>
				) : (
					<span className="hint">세션 헤더 대기 중…</span>
				)}
			</div>

			<div className="filterbar" style={{ marginBottom: 0 }}>
				<Legend
					items={GAUGE_SERIES.map((s) => ({
						id: s.key,
						label: s.label,
						color: seriesColor(s.key, theme),
						dash: s.dash,
					}))}
					hidden={hiddenSeries}
					onToggle={(id) =>
						setHiddenSeries((prev) => {
							const next = new Set(prev);
							if (next.has(id)) next.delete(id);
							else next.add(id);
							return next;
						})
					}
				/>
				<span className="spacer" />
				{(['gauge', 'ability', 'tag', 'marker'] as LaneId[]).map((id) => (
					<button
						key={id}
						aria-pressed={lanes[id]}
						onClick={() => setLanes((l) => ({ ...l, [id]: !l[id] }))}
						title="레인 표시/숨김"
					>
						{{ gauge: '게이지', ability: '어빌리티', tag: '태그', marker: '마커' }[id]}
					</button>
				))}
			</div>

			<div
				ref={wrapRef}
				className={`canvas-wrap${dragOver ? ' dragover' : ''}`}
				tabIndex={0}
				onKeyDown={onKeyDown}
				onWheel={onWheel}
				onDragOver={(e) => {
					e.preventDefault();
					setDragOver(true);
				}}
				onDragLeave={() => setDragOver(false)}
				onDrop={onDrop}
			>
				<canvas
					ref={canvasRef}
					onPointerDown={onPointerDown}
					onPointerMove={onPointerMove}
					onPointerLeave={() => setHover(null)}
				/>
				{hover && tooltip && (
					<div
						className="tooltip"
						style={{
							left: Math.min(hover.x + 14, size.w - 300),
							top: Math.min(hover.y + 14, Math.max(10, size.h - 240)),
						}}
					>
						<div className="tooltip__t">t = {hover.t.toFixed(3)}s</div>
						{tooltip.gauges.map((g) => (
							<div className="tooltip__row" key={g.id}>
								<span className="legend__dot" style={{ background: g.color }} />
								<span className="tooltip__label">
									{g.src === 'boss' ? 'BOSS ' : ''}
									{g.label}
								</span>
								<span>
									{(g.norm * 100).toFixed(0)}%{g.raw !== undefined ? ` (${g.raw.toFixed(0)})` : ''}
								</span>
							</div>
						))}
						{tooltip.abilities.length > 0 && (
							<>
								<div className="tooltip__sep" />
								{tooltip.abilities.map((a, i) => (
									<div className="tooltip__row" key={i}>
										<span className="tooltip__label">{a.src === 'boss' ? 'BOSS' : 'PLAYER'}</span>
										<span>{a.key}</span>
									</div>
								))}
							</>
						)}
						{tooltip.tags.length > 0 && (
							<>
								<div className="tooltip__sep" />
								{tooltip.tags.map((a, i) => (
									<div className="tooltip__row" key={i}>
										<span className="legend__dot" style={{ background: tagBarColor(a.key) }} />
										<span className="tooltip__label">{a.src === 'boss' ? 'BOSS' : 'PLAYER'}</span>
										<span>{shortTagPath(a.key)}</span>
									</div>
								))}
							</>
						)}
						{tooltip.near.length > 0 && (
							<>
								<div className="tooltip__sep" />
								{tooltip.near.map((n, i) => (
									<div className="tooltip__row" key={i}>
										<span>{n}</span>
									</div>
								))}
							</>
						)}
					</div>
				)}
			</div>

			<div className="scrub">
				<button onClick={() => setPlaying((p) => !p)} title="스페이스바">
					{playing ? '❚❚ 정지' : '▶ 라이브'}
				</button>
				<span className="mono">{playhead.toFixed(3)}s</span>
				<input
					type="range"
					min={0}
					max={tEnd}
					step={0.001}
					value={playhead}
					onChange={(e) => {
						setPlaying(false);
						moveHead(Number(e.target.value));
					}}
				/>
				<span className="mono">{tEnd.toFixed(1)}s</span>
				<button onClick={() => setView(clampView(0, tEnd))}>전체 보기</button>
				<span className="hint">드래그 스크럽 · ←/→ 1프레임 · Shift+←/→ 10프레임 · 휠 줌 · 스냅샷 JSON 드롭</span>
			</div>
		</div>
	);
}

function shortTagPath(tag: string): string {
	const parts = tag.split('.');
	return parts.length <= 2 ? tag : parts.slice(-2).join('.');
}

/* ── 툴팁 데이터 ─────────────────────────────────────────── */

function buildTooltip(model: Model, t: number, hidden: Set<string>) {
	const gauges: { id: string; src: string; label: string; color: string; norm: number; raw?: number }[] = [];
	for (const spec of GAUGE_SERIES) {
		if (hidden.has(spec.key)) continue;
		for (const src of ['player', 'boss'] as const) {
			const track = model.gauges.get(`${src}:${spec.key}`);
			if (!track || !track.t.length) continue;
			const i = lowerIndex(track.t, t);
			if (i < 0) continue;
			gauges.push({
				id: `${src}:${spec.key}`,
				src,
				label: spec.label,
				color: seriesColor(spec.key, 'dark'),
				norm: track.norm[i],
				raw: track.raw[i],
			});
		}
	}
	const abilities = model.abilities.filter((s) => t >= s.t0 && t <= s.t1);
	const tags = model.tags.filter((s) => t >= s.t0 && t <= s.t1);
	const near: string[] = [];
	for (const m of model.markers) {
		if (Math.abs(m.t - t) <= 0.15) {
			const style = MARKER_STYLE[m.key as MarkerKey];
			near.push(`${style?.glyph ?? '•'} ${style?.label ?? m.key} (${m.src})`);
		}
	}
	for (const h of model.hits) {
		if (Math.abs(h.t - t) <= 0.15) {
			const meta = h.meta;
			near.push(
				`● 피격 ${h.src} ${h.raw?.toFixed(0) ?? '?'} dmg` +
					(meta ? ` · ${meta.direction} · ${meta.sourceAbility}${meta.wasBlocked ? ' · 블록' : ''}${meta.wasParried ? ' · 패링' : ''}` : '')
			);
		}
	}
	return { gauges, abilities, tags, near };
}

/* ── 캔버스 드로잉 ───────────────────────────────────────── */

interface DrawArgs {
	w: number;
	h: number;
	view: { t0: number; t1: number };
	model: Model;
	theme: ThemeMode;
	lanes: Record<LaneId, boolean>;
	hiddenSeries: Set<string>;
	playhead: number;
	hoverT: number | null;
}

function draw(ctx: CanvasRenderingContext2D, a: DrawArgs): void {
	const ink = CHROME[a.theme];
	const { w, h, view, model } = a;
	const plotW = w - GUTTER - PAD_RIGHT;
	const spanT = view.t1 - view.t0;
	const X = (t: number) => GUTTER + ((t - view.t0) / spanT) * plotW;

	ctx.clearRect(0, 0, w, h);
	ctx.fillStyle = ink.surface;
	ctx.fillRect(0, 0, w, h);
	ctx.font = '11px system-ui, -apple-system, "Segoe UI", "Malgun Gothic", sans-serif';
	ctx.textBaseline = 'middle';

	// ── 레인 높이 배분
	const visible = (['gauge', 'ability', 'tag', 'marker'] as LaneId[]).filter((id) => a.lanes[id]);
	const fixed = { ability: 76, tag: Math.min(150, 18 + model.tagRows.length * 15), marker: 46 };
	const fixedTotal = visible.filter((id) => id !== 'gauge').reduce((s, id) => s + fixed[id as 'ability' | 'tag' | 'marker'], 0);
	const gaugeH = visible.includes('gauge') ? Math.max(140, h - AXIS_H - fixedTotal - 8) : 0;

	const bounds: Record<string, { y: number; h: number }> = {};
	let y = 6;
	for (const id of visible) {
		const lh = id === 'gauge' ? gaugeH : fixed[id as 'ability' | 'tag' | 'marker'];
		bounds[id] = { y, h: lh };
		y += lh + 2;
	}
	const axisY = Math.min(h - AXIS_H, y);

	// ── 시간 격자 (배경으로 물러나게)
	const step = niceStep(spanT, plotW);
	ctx.strokeStyle = ink.grid;
	ctx.lineWidth = 1;
	ctx.fillStyle = ink.textMuted;
	ctx.textAlign = 'center';
	for (let t = Math.ceil(view.t0 / step) * step; t <= view.t1; t += step) {
		const x = Math.round(X(t)) + 0.5;
		ctx.beginPath();
		ctx.moveTo(x, 6);
		ctx.lineTo(x, axisY);
		ctx.stroke();
		ctx.fillText(`${t.toFixed(step < 1 ? 1 : 0)}s`, x, axisY + AXIS_H / 2);
	}

	ctx.save();
	ctx.beginPath();
	ctx.rect(GUTTER, 0, plotW, h);
	ctx.clip();

	if (a.lanes.gauge) drawGaugeLane(ctx, a, bounds.gauge, X, ink);
	if (a.lanes.ability) drawAbilityLane(ctx, a, bounds.ability, X, ink);
	if (a.lanes.tag) drawTagLane(ctx, a, bounds.tag, X, ink);
	if (a.lanes.marker) drawMarkerLane(ctx, a, bounds.marker, X, ink);

	// ── 마커/피격 수직선 — 전 레인을 관통한다
	for (const m of model.markers) {
		if (m.t < view.t0 || m.t > view.t1) continue;
		const style = MARKER_STYLE[m.key as MarkerKey];
		ctx.strokeStyle = style?.color ?? ink.textMuted;
		ctx.globalAlpha = 0.35;
		ctx.lineWidth = 1;
		ctx.beginPath();
		ctx.moveTo(Math.round(X(m.t)) + 0.5, 6);
		ctx.lineTo(Math.round(X(m.t)) + 0.5, axisY);
		ctx.stroke();
		ctx.globalAlpha = 1;
	}

	// ── 재생 헤드 & 크로스헤어
	ctx.strokeStyle = ink.textPrimary;
	ctx.lineWidth = 1;
	ctx.beginPath();
	ctx.moveTo(Math.round(X(a.playhead)) + 0.5, 4);
	ctx.lineTo(Math.round(X(a.playhead)) + 0.5, axisY);
	ctx.stroke();

	if (a.hoverT !== null) {
		ctx.strokeStyle = ink.textMuted;
		ctx.setLineDash([3, 3]);
		ctx.beginPath();
		ctx.moveTo(Math.round(X(a.hoverT)) + 0.5, 4);
		ctx.lineTo(Math.round(X(a.hoverT)) + 0.5, axisY);
		ctx.stroke();
		ctx.setLineDash([]);
	}

	ctx.restore();

	// ── 축선
	ctx.strokeStyle = ink.axis;
	ctx.beginPath();
	ctx.moveTo(GUTTER, Math.round(axisY) + 0.5);
	ctx.lineTo(w - PAD_RIGHT, Math.round(axisY) + 0.5);
	ctx.stroke();
}

/** 레인1 — 게이지. PLAYER / BOSS 두 구획이 같은 0~100% 축을 쓴다(축은 하나만). */
function drawGaugeLane(
	ctx: CanvasRenderingContext2D,
	a: DrawArgs,
	box: { y: number; h: number },
	X: (t: number) => number,
	ink: (typeof CHROME)['dark']
): void {
	const bossH = Math.min(88, Math.max(56, box.h * 0.3));
	const playerBox = { y: box.y, h: box.h - bossH - 6 };
	const bossBox = { y: box.y + playerBox.h + 6, h: bossH };

	for (const [label, sub] of [
		['PLAYER', playerBox],
		['BOSS', bossBox],
	] as const) {
		// 0/50/100% 눈금 — 배경으로 물러나게
		ctx.strokeStyle = ink.grid;
		ctx.lineWidth = 1;
		ctx.textAlign = 'right';
		ctx.fillStyle = ink.textMuted;
		for (const frac of [0, 0.5, 1]) {
			const yy = Math.round(sub.y + sub.h * (1 - frac)) + 0.5;
			ctx.beginPath();
			ctx.moveTo(GUTTER, yy);
			ctx.lineTo(ctx.canvas.width, yy);
			ctx.stroke();
			ctx.fillText(`${frac * 100}%`, GUTTER - 8, yy);
		}
		ctx.textAlign = 'left';
		ctx.fillStyle = ink.textSecondary;
		ctx.fillText(label, 6, sub.y + 8);
	}

	for (const spec of GAUGE_SERIES) {
		if (a.hiddenSeries.has(spec.key)) continue;
		for (const [src, sub] of [
			['player', playerBox],
			['boss', bossBox],
		] as const) {
			const track = a.model.gauges.get(`${src}:${spec.key}`);
			if (!track || track.t.length < 1) continue;
			drawTrack(ctx, track, sub, X, a.view, seriesColor(spec.key, a.theme), spec.dash);
		}
	}
}

/** 보이는 구간만, 픽셀당 표본이 넘치면 min/max 로 접어 그린다 */
function drawTrack(
	ctx: CanvasRenderingContext2D,
	track: GaugeTrack,
	box: { y: number; h: number },
	X: (t: number) => number,
	view: { t0: number; t1: number },
	color: string,
	dash: number[]
): void {
	const from = Math.max(0, lowerIndex(track.t, view.t0));
	let to = lowerIndex(track.t, view.t1);
	if (to < 0) return;
	to = Math.min(track.t.length - 1, to + 1);
	if (to <= from) return;

	const Y = (n: number) => box.y + box.h * (1 - Math.max(0, Math.min(1, n)));

	ctx.strokeStyle = color;
	ctx.lineWidth = 2;
	ctx.lineJoin = 'round';
	ctx.setLineDash(dash);
	ctx.beginPath();

	const count = to - from + 1;
	const pxSpan = Math.max(1, X(view.t1) - X(view.t0));
	if (count <= pxSpan * 1.5) {
		// 표본이 픽셀보다 적다 — 그대로 계단(step-after)으로 잇는다
		let px = X(track.t[from]);
		let py = Y(track.norm[from]);
		ctx.moveTo(px, py);
		for (let i = from + 1; i <= to; i++) {
			const x = X(track.t[i]);
			const yv = Y(track.norm[i]);
			ctx.lineTo(x, py);
			ctx.lineTo(x, yv);
			px = x;
			py = yv;
		}
	} else {
		// 픽셀 열마다 min/max 만 남긴다
		const bucket = (count - 1) / pxSpan;
		let i = from;
		let first = true;
		for (let col = 0; col <= pxSpan; col++) {
			const end = Math.min(to, from + Math.round((col + 1) * bucket));
			if (end < i) continue;
			let mn = track.norm[i];
			let mx = track.norm[i];
			for (let k = i; k <= end; k++) {
				const v = track.norm[k];
				if (v < mn) mn = v;
				if (v > mx) mx = v;
			}
			const x = X(track.t[i]);
			if (first) {
				ctx.moveTo(x, Y(mx));
				first = false;
			}
			ctx.lineTo(x, Y(mx));
			ctx.lineTo(x, Y(mn));
			i = end + 1;
			if (i > to) break;
		}
	}
	ctx.stroke();
	ctx.setLineDash([]);
}

/** 레인2 — 어빌리티 활성 구간. player 행 / boss 행 분리 */
function drawAbilityLane(
	ctx: CanvasRenderingContext2D,
	a: DrawArgs,
	box: { y: number; h: number },
	X: (t: number) => number,
	ink: (typeof CHROME)['dark']
): void {
	const rowH = (box.h - 6) / 2;
	const rows: Array<['player' | 'boss', number, string]> = [
		['player', box.y, ink.lanePlayer],
		['boss', box.y + rowH + 6, ink.laneBoss],
	];
	for (const [src, ry, bg] of rows) {
		ctx.fillStyle = bg;
		ctx.fillRect(GUTTER, ry, ctx.canvas.width, rowH);
		ctx.fillStyle = ink.textSecondary;
		ctx.textAlign = 'left';
		ctx.fillText(src === 'player' ? 'PLAYER' : 'BOSS', 6, ry + rowH / 2);

		let lane = 0;
		const laneEnds: number[] = [];
		for (const s of a.model.abilities) {
			if (s.src !== src || s.t1 < a.view.t0 || s.t0 > a.view.t1) continue;
			// 겹치는 구간은 아래 줄로 밀어 그린다
			lane = laneEnds.findIndex((e) => e <= s.t0);
			if (lane === -1) {
				lane = laneEnds.length;
				laneEnds.push(s.t1);
			} else {
				laneEnds[lane] = s.t1;
			}
			const subH = Math.max(9, rowH / Math.max(1, laneEnds.length));
			const by = ry + lane * subH + 1;
			const x0 = X(s.t0);
			const x1 = Math.max(x0 + 2, X(s.t1));
			ctx.fillStyle = src === 'player' ? '#3987e5' : '#d95926';
			ctx.globalAlpha = 0.85;
			roundRect(ctx, x0, by, x1 - x0, subH - 2, 3);
			ctx.fill();
			ctx.globalAlpha = 1;
			if (x1 - x0 > 46) {
				ctx.save();
				ctx.beginPath();
				ctx.rect(x0 + 3, by, x1 - x0 - 6, subH - 2);
				ctx.clip();
				ctx.fillStyle = ink.textPrimary;
				ctx.fillText(s.key.replace(/^GA_/, ''), x0 + 5, by + (subH - 2) / 2);
				ctx.restore();
			}
		}
	}
}

/** 레인3 — 태그 부여 구간 */
function drawTagLane(
	ctx: CanvasRenderingContext2D,
	a: DrawArgs,
	box: { y: number; h: number },
	X: (t: number) => number,
	ink: (typeof CHROME)['dark']
): void {
	const rows = a.model.tagRows;
	const rowH = Math.max(11, Math.min(15, (box.h - 4) / Math.max(1, rows.length)));
	rows.forEach((rowId, i) => {
		const ry = box.y + i * rowH;
		if (ry + rowH > box.y + box.h) return;
		const [src, ...rest] = rowId.split(':');
		const key = rest.join(':');
		ctx.fillStyle = ink.textMuted;
		ctx.textAlign = 'right';
		ctx.save();
		ctx.beginPath();
		ctx.rect(0, ry, GUTTER - 4, rowH);
		ctx.clip();
		ctx.fillText(`${src === 'boss' ? 'B ' : 'P '}${shortTagPath(key)}`, GUTTER - 6, ry + rowH / 2);
		ctx.restore();
		ctx.textAlign = 'left';

		for (const s of a.model.tags) {
			if (`${s.src}:${s.key}` !== rowId) continue;
			if (s.t1 < a.view.t0 || s.t0 > a.view.t1) continue;
			const x0 = X(s.t0);
			const x1 = Math.max(x0 + 2, X(s.t1));
			ctx.fillStyle = tagBarColor(s.key);
			roundRect(ctx, x0, ry + 1.5, x1 - x0, rowH - 3, 2);
			ctx.fill();
		}
	});
}

/** 레인4 — 이벤트 마커. 색 외에 글리프로도 구분한다 */
function drawMarkerLane(
	ctx: CanvasRenderingContext2D,
	a: DrawArgs,
	box: { y: number; h: number },
	X: (t: number) => number,
	ink: (typeof CHROME)['dark']
): void {
	ctx.fillStyle = ink.textSecondary;
	ctx.textAlign = 'left';
	ctx.fillText('EVENT', 6, box.y + box.h / 2);

	ctx.textAlign = 'center';
	ctx.font = '13px system-ui, -apple-system, "Segoe UI", sans-serif';
	for (const h of a.model.hits) {
		if (h.t < a.view.t0 || h.t > a.view.t1) continue;
		const x = X(h.t);
		ctx.fillStyle = h.meta?.wasBlocked ? ink.textMuted : HIT_COLOR;
		ctx.beginPath();
		ctx.arc(x, box.y + box.h * (h.src === 'player' ? 0.35 : 0.72), 3.5, 0, Math.PI * 2);
		ctx.fill();
	}
	for (const m of a.model.markers) {
		if (m.t < a.view.t0 || m.t > a.view.t1) continue;
		const style = MARKER_STYLE[m.key as MarkerKey];
		ctx.fillStyle = style?.color ?? ink.textPrimary;
		ctx.fillText(style?.glyph ?? '•', X(m.t), box.y + box.h * (m.src === 'player' ? 0.35 : 0.72));
	}
	ctx.font = '11px system-ui, -apple-system, "Segoe UI", "Malgun Gothic", sans-serif';
	ctx.textAlign = 'left';
}

function roundRect(ctx: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, r: number): void {
	const rr = Math.min(r, w / 2, h / 2);
	ctx.beginPath();
	ctx.moveTo(x + rr, y);
	ctx.arcTo(x + w, y, x + w, y + h, rr);
	ctx.arcTo(x + w, y + h, x, y + h, rr);
	ctx.arcTo(x, y + h, x, y, rr);
	ctx.arcTo(x, y, x + w, y, rr);
	ctx.closePath();
}

/** 눈금 간격을 1·2·5 계열로 고른다 */
function niceStep(spanT: number, plotW: number): number {
	const target = spanT / Math.max(2, plotW / 90);
	const mag = Math.pow(10, Math.floor(Math.log10(target)));
	for (const m of [1, 2, 5, 10]) {
		if (target <= mag * m) return mag * m;
	}
	return mag * 10;
}
