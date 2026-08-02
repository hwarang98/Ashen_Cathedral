// 런 통계 화면 — Saved/RunLogs/*.json 을 읽어 렌더한다.
// 게임이 실행 중이 아니어도 동작해야 하므로 /api/runs 실패 시 mock/runs/*.json 으로 폴백한다.

import { Fragment, useEffect, useMemo, useState } from 'react';
import {
	Bar,
	BarChart,
	CartesianGrid,
	Cell,
	Line,
	LineChart,
	ReferenceArea,
	ReferenceLine,
	ResponsiveContainer,
	Tooltip,
	XAxis,
	YAxis,
} from 'recharts';
import Panel from '../components/Panel';
import Legend from '../components/Legend';
import { fetchRun, fetchRunList } from '../lib/ws';
import { CHROME, STATUS, type ThemeMode } from '../lib/palette';
import { shortTag, type RunLog } from '../lib/schema';
import {
	EMPTY_FILTERS,
	cardPickRates,
	collectIframeHits,
	collectParryOffsets,
	deathCauses,
	histogram,
	learningCurve,
	mean,
	median,
	runRows,
	selectFights,
	selectRuns,
	type Filters,
} from '../lib/stats';

/** 통계 화면의 단일 계열 색 — 계열이 하나뿐이라 정체성 충돌이 없다 */
const SERIES_1 = { dark: '#3987e5', light: '#2a78d6' };

interface Props {
	theme: ThemeMode;
}

export default function RunStats({ theme }: Props) {
	const ink = CHROME[theme];
	const accent = theme === 'dark' ? SERIES_1.dark : SERIES_1.light;

	const [runs, setRuns] = useState<RunLog[]>([]);
	const [loading, setLoading] = useState(true);
	const [fromMock, setFromMock] = useState(false);
	const [error, setError] = useState<string | null>(null);
	const [filters, setFilters] = useState<Filters>(EMPTY_FILTERS);
	const [expanded, setExpanded] = useState<string | null>(null);

	useEffect(() => {
		let cancelled = false;
		(async () => {
			try {
				const { entries, mock } = await fetchRunList();
				const loaded = await Promise.all(
					entries.map((e) => fetchRun(e, mock).catch(() => null))
				);
				if (cancelled) return;
				setRuns(loaded.filter(Boolean) as RunLog[]);
				setFromMock(mock);
			} catch (e) {
				if (!cancelled) setError(String(e));
			} finally {
				if (!cancelled) setLoading(false);
			}
		})();
		return () => {
			cancelled = true;
		};
	}, []);

	const bossIds = useMemo(
		() => [...new Set(runs.flatMap((r) => r.bossFights.map((f) => f.bossId)))].sort(),
		[runs]
	);
	const weapons = useMemo(() => [...new Set(runs.map((r) => r.weapon))].sort(), [runs]);

	const fights = useMemo(() => selectFights(runs, filters), [runs, filters]);
	const filteredRuns = useMemo(() => selectRuns(runs, filters), [runs, filters]);

	const curve = useMemo(() => learningCurve(fights), [fights]);
	const parrySamples = useMemo(() => collectParryOffsets(fights), [fights]);
	const parryBins = useMemo(() => histogram(parrySamples, 25, -400, 300), [parrySamples]);
	const iframeSamples = useMemo(() => collectIframeHits(fights), [fights]);
	const iframeBins = useMemo(() => histogram(iframeSamples, 25, 0, 600), [iframeSamples]);
	const causes = useMemo(() => deathCauses(fights, 8), [fights]);
	const cards = useMemo(() => cardPickRates(filteredRuns), [filteredRuns]);
	const rows = useMemo(() => runRows(filteredRuns), [filteredRuns]);

	if (loading) return <div className="empty">런 로그를 읽는 중…</div>;
	if (error) return <div className="empty">런 로그를 읽지 못했습니다 — {error}</div>;
	if (!runs.length) return <div className="empty">런 로그가 없습니다. Saved/RunLogs/ 를 확인하세요.</div>;

	const axis = { stroke: ink.axis, tick: { fill: ink.textMuted, fontSize: 11 } };
	const tooltipStyle = {
		background: ink.plane,
		border: `1px solid ${ink.border}`,
		borderRadius: 6,
		color: ink.textPrimary,
		fontSize: 11,
	};

	const parryMean = mean(parrySamples);
	const parryMedian = median(parrySamples);
	const iframeNear = iframeSamples.filter((v) => v <= 50).length;

	return (
		<div className="grid" style={{ gap: 12 }}>
			{/* ── 필터 (한 줄) ─────────────────────────────────────── */}
			<div className="filterbar">
				<label className="field">
					보스
					<select value={filters.bossId} onChange={(e) => setFilters({ ...filters, bossId: e.target.value })}>
						<option value="">전체</option>
						{bossIds.map((b) => (
							<option key={b} value={b}>
								{shortTag(b)}
							</option>
						))}
					</select>
				</label>
				<label className="field">
					무기
					<select value={filters.weapon} onChange={(e) => setFilters({ ...filters, weapon: e.target.value })}>
						<option value="">전체</option>
						{weapons.map((w) => (
							<option key={w} value={w}>
								{shortTag(w)}
							</option>
						))}
					</select>
				</label>
				<label className="field">
					시도 범위
					<input
						type="number"
						style={{ width: 62 }}
						value={filters.attemptMin}
						onChange={(e) => setFilters({ ...filters, attemptMin: Number(e.target.value) || 0 })}
					/>
					<span style={{ color: 'var(--text-muted)' }}>–</span>
					<input
						type="number"
						style={{ width: 62 }}
						value={filters.attemptMax}
						onChange={(e) => setFilters({ ...filters, attemptMax: Number(e.target.value) || 9999 })}
					/>
				</label>
				<label className="field">
					결과
					<select
						value={filters.result}
						onChange={(e) => setFilters({ ...filters, result: e.target.value as Filters['result'] })}
					>
						<option value="">전체</option>
						<option value="cleared">클리어</option>
						<option value="died">사망</option>
					</select>
				</label>
				<button onClick={() => setFilters(EMPTY_FILTERS)}>초기화</button>
				<span className="spacer" />
				<span className="hint">
					런 {filteredRuns.length} / {runs.length} · 보스전 {fights.length}
					{fromMock && ' · mock 데이터'}
				</span>
			</div>

			<div className="grid grid--2">
				{/* ── ① 학습 곡선 ────────────────────────────────── */}
				<Panel
					title="① 학습 곡선"
					note="내려가면 학습 중, 평평하면 보스 패턴을 못 읽고 있다는 뜻이다."
					aside={
						<Legend
							items={[
								{ id: 'v', label: '보스 잔여 HP', color: accent },
								{ id: 't', label: '추세선', color: ink.textMuted, dash: [5, 4] },
							]}
						/>
					}
				>
					<ResponsiveContainer width="100%" height={220}>
						<LineChart data={curve} margin={{ top: 8, right: 12, bottom: 4, left: -8 }}>
							<CartesianGrid stroke={ink.grid} vertical={false} />
							<XAxis dataKey="attempt" {...axis} label={undefined} />
							<YAxis
								domain={[0, 1]}
								tickFormatter={(v: number) => `${Math.round(v * 100)}%`}
								{...axis}
							/>
							<Tooltip
								contentStyle={tooltipStyle}
								formatter={(v: number, name: string) => [`${(v * 100).toFixed(1)}%`, name]}
								labelFormatter={(l) => `${l}회차`}
							/>
							<Line
								type="monotone"
								dataKey="bossHealthPctAtEnd"
								name="보스 잔여 HP"
								stroke={accent}
								strokeWidth={2}
								dot={{ r: 3, fill: accent, stroke: 'none' }}
								isAnimationActive={false}
							/>
							<Line
								type="linear"
								dataKey="trend"
								name="추세선"
								stroke={ink.textMuted}
								strokeWidth={2}
								strokeDasharray="5 4"
								dot={false}
								isAnimationActive={false}
							/>
						</LineChart>
					</ResponsiveContainer>
				</Panel>

				{/* ── ② 패링 입력 타이밍 분포 ──────────────────────── */}
				<Panel
					title="② 패링 입력 타이밍 분포"
					note={`0 = 판정 시작 시각. 음영이 0.25s 판정 윈도우다. 평균 ${parryMean.toFixed(0)}ms · 중앙값 ${parryMedian.toFixed(0)}ms — 중심이 음수 쪽이면 윈도우를 앞으로 옮겨야 한다.`}
				>
					<ResponsiveContainer width="100%" height={220}>
						<BarChart data={parryBins} margin={{ top: 8, right: 12, bottom: 4, left: -8 }}>
							<CartesianGrid stroke={ink.grid} vertical={false} />
							<ReferenceArea
								x1={0}
								x2={250}
								fill={STATUS.good}
								fillOpacity={0.12}
								stroke="none"
								label={{ value: '판정 윈도우 0.25s', fill: ink.textMuted, fontSize: 10, position: 'insideTop' }}
							/>
							<ReferenceLine x={0} stroke={ink.axis} strokeWidth={1} />
							<XAxis
								dataKey="center"
								type="number"
								domain={[-400, 300]}
								ticks={[-400, -300, -200, -100, 0, 100, 200, 300]}
								tickFormatter={(v: number) => `${v}`}
								{...axis}
							/>
							<YAxis allowDecimals={false} {...axis} />
							<Tooltip
								contentStyle={tooltipStyle}
								formatter={(v: number) => [`${v}회`, '표본']}
								labelFormatter={(l: number) => `${l - 12.5}~${l + 12.5}ms`}
							/>
							<Bar dataKey="count" fill={accent} radius={[3, 3, 0, 0]} isAnimationActive={false} />
						</BarChart>
					</ResponsiveContainer>
					<p className="hint" style={{ marginTop: 6 }}>
						표본 {parrySamples.length}개 · 음수 = 일찍 누름
					</p>
				</Panel>

				{/* ── ③ 무적 종료 후 피격 분포 ─────────────────────── */}
				<Panel
					title="③ 무적 종료 후 피격 분포"
					note={`0~50ms 구간에 몰려 있으면 무적 프레임이 짧다는 뜻이다. 현재 ${iframeSamples.length}개 중 ${iframeNear}개(${iframeSamples.length ? Math.round((iframeNear / iframeSamples.length) * 100) : 0}%)가 이 구간에 있다.`}
				>
					<ResponsiveContainer width="100%" height={220}>
						<BarChart data={iframeBins} margin={{ top: 8, right: 12, bottom: 4, left: -8 }}>
							<CartesianGrid stroke={ink.grid} vertical={false} />
							<ReferenceArea
								x1={0}
								x2={50}
								fill={STATUS.critical}
								fillOpacity={0.14}
								stroke="none"
								label={{ value: '0–50ms', fill: ink.textMuted, fontSize: 10, position: 'insideTop' }}
							/>
							<XAxis
								dataKey="center"
								type="number"
								domain={[0, 600]}
								ticks={[0, 100, 200, 300, 400, 500, 600]}
								{...axis}
							/>
							<YAxis allowDecimals={false} {...axis} />
							<Tooltip
								contentStyle={tooltipStyle}
								formatter={(v: number) => [`${v}회`, '표본']}
								labelFormatter={(l: number) => `${l - 12.5}~${l + 12.5}ms`}
							/>
							<Bar dataKey="count" fill={accent} radius={[3, 3, 0, 0]} isAnimationActive={false} />
						</BarChart>
					</ResponsiveContainer>
					<p className="hint" style={{ marginTop: 6 }}>
						양수 = 무적이 끝난 뒤 맞음 · 2초 이내 표본만 집계됨
					</p>
				</Panel>

				{/* ── ④ 사망 원인 분포 ─────────────────────────────── */}
				<Panel
					title="④ 사망 원인 분포"
					note="특정 패턴이 누적 피해의 대부분이면 그 패턴만 조정하면 된다. 막대는 누적 피해, 표는 실제 사망타 횟수."
				>
					<ResponsiveContainer width="100%" height={Math.max(160, causes.length * 26 + 30)}>
						<BarChart data={causes} layout="vertical" margin={{ top: 4, right: 16, bottom: 4, left: 4 }}>
							<CartesianGrid stroke={ink.grid} horizontal={false} />
							<XAxis type="number" {...axis} />
							<YAxis
								type="category"
								dataKey="tag"
								width={150}
								tickFormatter={shortTag}
								{...axis}
							/>
							<Tooltip
								contentStyle={tooltipStyle}
								formatter={(v: number) => [`${v.toLocaleString()}`, '누적 피해']}
								labelFormatter={shortTag}
							/>
							<Bar dataKey="damage" fill={accent} radius={[0, 3, 3, 0]} isAnimationActive={false} />
						</BarChart>
					</ResponsiveContainer>
					<div className="table-scroll" style={{ marginTop: 8, maxHeight: 180 }}>
						<table>
							<thead>
								<tr>
									<th>공격 태그</th>
									<th className="num">누적 피해</th>
									<th className="num">사망타</th>
								</tr>
							</thead>
							<tbody>
								{causes.map((c) => (
									<tr key={c.tag}>
										<td>{shortTag(c.tag)}</td>
										<td className="num">{c.damage.toLocaleString()}</td>
										<td className="num">{c.deaths}</td>
									</tr>
								))}
							</tbody>
						</table>
					</div>
				</Panel>

				{/* ── ⑤ 카드 픽률 ──────────────────────────────────── */}
				<Panel
					title="⑤ 카드 픽률"
					note="픽률 0%인 카드는 존재하지 않는 것과 같다. 제시 횟수는 선택된 카드와 함께 제시된 카드를 모두 센다."
				>
					<ResponsiveContainer width="100%" height={Math.max(180, cards.length * 22 + 30)}>
						<BarChart data={cards} layout="vertical" margin={{ top: 4, right: 40, bottom: 4, left: 4 }}>
							<CartesianGrid stroke={ink.grid} horizontal={false} />
							<XAxis
								type="number"
								domain={[0, 1]}
								tickFormatter={(v: number) => `${Math.round(v * 100)}%`}
								{...axis}
							/>
							<YAxis type="category" dataKey="cardId" width={54} {...axis} />
							<Tooltip
								contentStyle={tooltipStyle}
								formatter={(v: number, _n, p) => [
									`${(v * 100).toFixed(0)}%  (${p.payload.picked}/${p.payload.offered})`,
									'픽률',
								]}
							/>
							<Bar dataKey="pickRate" radius={[0, 3, 3, 0]} isAnimationActive={false}>
								{cards.map((c) => (
									// 픽률 0 인 카드만 위험색으로 — 색 외에 표의 0/N 값이 함께 근거를 준다
									<Cell key={c.cardId} fill={c.picked === 0 ? STATUS.critical : accent} />
								))}
							</Bar>
						</BarChart>
					</ResponsiveContainer>
					<div className="table-scroll" style={{ marginTop: 8, maxHeight: 200 }}>
						<table>
							<thead>
								<tr>
									<th>카드</th>
									<th>계열</th>
									<th>희귀도</th>
									<th className="num">제시</th>
									<th className="num">선택</th>
									<th className="num">픽률</th>
								</tr>
							</thead>
							<tbody>
								{cards.map((c) => (
									<tr key={c.cardId}>
										<td>{c.cardId}</td>
										<td>{c.category}</td>
										<td>{c.rarity}</td>
										<td className="num">{c.offered}</td>
										<td className="num">{c.picked}</td>
										<td className="num">{(c.pickRate * 100).toFixed(0)}%</td>
									</tr>
								))}
							</tbody>
						</table>
					</div>
				</Panel>
			</div>

			{/* ── 런 목록 ────────────────────────────────────────── */}
			<Panel title="런 목록" note="행을 클릭하면 카드 목록과 보스전별 지표가 펼쳐진다.">
				<div className="table-scroll">
					<table>
						<thead>
							<tr>
								<th>런</th>
								<th>시작(UTC)</th>
								<th>무기</th>
								<th>결과</th>
								<th className="num">길이</th>
								<th className="num">보스전</th>
								<th className="num">최저 보스 HP</th>
								<th className="num">패링률</th>
								<th className="num">받은 피해</th>
								<th className="num">카드</th>
							</tr>
						</thead>
						<tbody>
							{rows.map((r) => (
								<Fragment key={r.runId}>
									<tr
										className="clickable"
										onClick={() => setExpanded(expanded === r.runId ? null : r.runId)}
									>
										<td>{expanded === r.runId ? '▾ ' : '▸ '}{r.runId}</td>
										<td>{r.startedAtUtc.replace('T', ' ').replace('Z', '')}</td>
										<td>{shortTag(r.weapon)}</td>
										<td>{r.result === 'cleared' ? '클리어' : r.result === 'died' ? '사망' : r.result}</td>
										<td className="num">{Math.round(r.durationSec)}s</td>
										<td className="num">{r.fights}</td>
										<td className="num">{(r.bestBossHpPct * 100).toFixed(1)}%</td>
										<td className="num">{(r.parryRate * 100).toFixed(0)}%</td>
										<td className="num">{r.totalDamageTaken.toLocaleString()}</td>
										<td className="num">{r.cards}</td>
									</tr>
									{expanded === r.runId && (
										<tr>
											<td colSpan={10} style={{ whiteSpace: 'normal', padding: 12 }}>
												<RunDetail run={r.run} />
											</td>
										</tr>
									)}
								</Fragment>
							))}
						</tbody>
					</table>
				</div>
			</Panel>
		</div>
	);
}

function RunDetail({ run }: { run: RunLog }) {
	return (
		<div className="grid" style={{ gap: 10 }}>
			<div>
				<b>카드</b>{' '}
				{run.cardsPicked.length === 0 ? (
					<span className="hint">선택한 카드 없음</span>
				) : (
					run.cardsPicked
						.map((c) => `${c.cardId}(${c.rarity}·x${c.stackAfter}) ← ${c.offeredWith.join('/')}`)
						.join(' · ')
				)}
			</div>
			<div>
				<b>재화</b>{' '}
				{Object.entries(run.currencyEarned)
					.map(([k, v]) => `${shortTag(k)} ${v}`)
					.join(' · ')}
				{'  |  '}
				<b>메타</b>{' '}
				{Object.entries(run.metaUpgrades)
					.map(([k, v]) => `${k} ${v}`)
					.join(' · ')}
			</div>
			<table>
				<thead>
					<tr>
						<th>보스</th>
						<th className="num">시도</th>
						<th>결과</th>
						<th className="num">길이</th>
						<th className="num">잔여 HP</th>
						<th className="num">패링</th>
						<th className="num">회피(무적)</th>
						<th className="num">블록</th>
						<th className="num">가드브레이크 피/가</th>
						<th className="num">체간붕괴 피/가</th>
						<th className="num">치명타</th>
						<th>사망 원인</th>
					</tr>
				</thead>
				<tbody>
					{run.bossFights.map((f, i) => (
						<tr key={`${f.bossId}-${i}`}>
							<td>{shortTag(f.bossId)}</td>
							<td className="num">{f.attempt}</td>
							<td>{f.result === 'won' ? '승리' : '패배'}</td>
							<td className="num">{Math.round(f.durationSec)}s</td>
							<td className="num">{(f.bossHealthPctAtEnd * 100).toFixed(1)}%</td>
							<td className="num">
								{f.parry.successes}/{f.parry.attempts}
							</td>
							<td className="num">
								{f.dodge.iframeSuccesses}/{f.dodge.attempts}
							</td>
							<td className="num">
								{f.block.successes}/{f.block.attempts}
							</td>
							<td className="num">
								{f.guardBreaksTaken}/{f.guardBreaksInflicted}
							</td>
							<td className="num">
								{f.postureBreaksTaken}/{f.postureBreaksInflicted}
							</td>
							<td className="num">{f.criticalAttacksLanded}</td>
							<td>{f.deathCause ? `${shortTag(f.deathCause.attackTag)} (${f.deathCause.damage})` : '—'}</td>
						</tr>
					))}
				</tbody>
			</table>
		</div>
	);
}
