// 런 로그 집계 — 화면(RunStats)은 여기서 나온 배열을 그리기만 한다.

import type { BossFight, CardPick, RunLog } from './schema';

export interface Filters {
	bossId: string; // '' = 전체
	weapon: string; // '' = 전체
	attemptMin: number;
	attemptMax: number;
	result: '' | 'cleared' | 'died';
}

export const EMPTY_FILTERS: Filters = { bossId: '', weapon: '', attemptMin: 0, attemptMax: 9999, result: '' };

/** 필터를 통과한 (런, 보스전) 쌍. 위젯 대부분은 보스전 단위로 집계한다. */
export interface FightRef {
	run: RunLog;
	fight: BossFight;
}

export function selectFights(runs: RunLog[], f: Filters): FightRef[] {
	const out: FightRef[] = [];
	for (const run of runs) {
		if (f.weapon && run.weapon !== f.weapon) continue;
		if (f.result && run.result !== f.result) continue;
		for (const fight of run.bossFights) {
			if (f.bossId && fight.bossId !== f.bossId) continue;
			if (fight.attempt < f.attemptMin || fight.attempt > f.attemptMax) continue;
			out.push({ run, fight });
		}
	}
	return out;
}

export function selectRuns(runs: RunLog[], f: Filters): RunLog[] {
	const keep = new Set(selectFights(runs, f).map((r) => r.run.runId));
	return runs.filter((r) => keep.has(r.runId));
}

/* ── ① 학습 곡선 ─────────────────────────────────────────── */

export interface LearningPoint {
	attempt: number;
	bossHealthPctAtEnd: number;
	trend: number;
	runId: string;
}

/** x=시도 횟수, y=bossHealthPctAtEnd. 추세선은 최소제곱 직선. */
export function learningCurve(fights: FightRef[]): LearningPoint[] {
	const pts = fights
		.map((r) => ({ attempt: r.fight.attempt, y: r.fight.bossHealthPctAtEnd, runId: r.run.runId }))
		.sort((a, b) => a.attempt - b.attempt);
	if (!pts.length) return [];

	const n = pts.length;
	const sx = pts.reduce((s, p) => s + p.attempt, 0);
	const sy = pts.reduce((s, p) => s + p.y, 0);
	const sxx = pts.reduce((s, p) => s + p.attempt * p.attempt, 0);
	const sxy = pts.reduce((s, p) => s + p.attempt * p.y, 0);
	const denom = n * sxx - sx * sx;
	const slope = denom === 0 ? 0 : (n * sxy - sx * sy) / denom;
	const intercept = (sy - slope * sx) / n;

	return pts.map((p) => ({
		attempt: p.attempt,
		bossHealthPctAtEnd: p.y,
		trend: Math.max(0, Math.min(1, intercept + slope * p.attempt)),
		runId: p.runId,
	}));
}

/* ── ②③ 히스토그램 ──────────────────────────────────────── */

export interface HistBin {
	/** 구간 하한 */
	x0: number;
	/** 구간 상한 */
	x1: number;
	/** 구간 중심 — 축 라벨용 */
	center: number;
	count: number;
}

export function histogram(samples: number[], binWidth: number, min?: number, max?: number): HistBin[] {
	if (!samples.length) return [];
	const lo = min ?? Math.floor(Math.min(...samples) / binWidth) * binWidth;
	const hi = max ?? Math.ceil(Math.max(...samples) / binWidth) * binWidth;
	const bins: HistBin[] = [];
	for (let x = lo; x < hi; x += binWidth) {
		bins.push({ x0: x, x1: x + binWidth, center: x + binWidth / 2, count: 0 });
	}
	if (!bins.length) return [];
	for (const s of samples) {
		const clamped = Math.max(lo, Math.min(hi - 1e-9, s));
		const i = Math.min(bins.length - 1, Math.floor((clamped - lo) / binWidth));
		bins[i].count++;
	}
	return bins;
}

export function collectParryOffsets(fights: FightRef[]): number[] {
	return fights.flatMap((r) => r.fight.timingSamples?.parryInputOffsetMs ?? []);
}

export function collectIframeHits(fights: FightRef[]): number[] {
	return fights.flatMap((r) => r.fight.timingSamples?.hitAfterIframeEndMs ?? []);
}

export function collectComboOffsets(fights: FightRef[]): number[] {
	return fights.flatMap((r) => r.fight.timingSamples?.comboWindowInputOffsetMs ?? []);
}

export function mean(v: number[]): number {
	return v.length ? v.reduce((s, x) => s + x, 0) / v.length : 0;
}

export function median(v: number[]): number {
	if (!v.length) return 0;
	const s = [...v].sort((a, b) => a - b);
	const m = s.length >> 1;
	return s.length % 2 ? s[m] : (s[m - 1] + s[m]) / 2;
}

/* ── ④ 사망 원인 ─────────────────────────────────────────── */

export interface DeathCauseRow {
	tag: string;
	damage: number;
	deaths: number;
}

/** damageTakenByAttackTag 를 전체 런에 걸쳐 합산하고 deathCause 빈도를 함께 센다 */
export function deathCauses(fights: FightRef[], topN = 8): DeathCauseRow[] {
	const dmg = new Map<string, number>();
	const deaths = new Map<string, number>();
	for (const { fight } of fights) {
		for (const [tag, amount] of Object.entries(fight.damageTakenByAttackTag ?? {})) {
			dmg.set(tag, (dmg.get(tag) ?? 0) + amount);
		}
		const cause = fight.deathCause?.attackTag;
		if (cause) deaths.set(cause, (deaths.get(cause) ?? 0) + 1);
	}
	const tags = new Set([...dmg.keys(), ...deaths.keys()]);
	return [...tags]
		.map((tag) => ({ tag, damage: Math.round(dmg.get(tag) ?? 0), deaths: deaths.get(tag) ?? 0 }))
		.sort((a, b) => b.damage - a.damage)
		.slice(0, topN);
}

/* ── ⑤ 카드 픽률 ─────────────────────────────────────────── */

export interface CardPickRow {
	cardId: string;
	category: string;
	rarity: string;
	offered: number;
	picked: number;
	pickRate: number;
}

/**
 * 제시 횟수는 선택된 카드 1장 + offeredWith 에 들어 있는 카드들을 모두 센다.
 * 픽률 0%인 카드는 존재하지 않는 것과 같으므로 offered 만 있는 카드도 행으로 남긴다.
 */
export function cardPickRates(runs: RunLog[]): CardPickRow[] {
	const offered = new Map<string, number>();
	const picked = new Map<string, number>();
	const meta = new Map<string, { category: string; rarity: string }>();

	const bump = (m: Map<string, number>, k: string) => m.set(k, (m.get(k) ?? 0) + 1);

	for (const run of runs) {
		for (const pick of run.cardsPicked as CardPick[]) {
			bump(offered, pick.cardId);
			bump(picked, pick.cardId);
			meta.set(pick.cardId, { category: pick.category, rarity: pick.rarity });
			for (const other of pick.offeredWith) {
				bump(offered, other);
				if (!meta.has(other)) meta.set(other, { category: '?', rarity: '?' });
			}
		}
	}

	return [...offered.keys()]
		.map((cardId) => {
			const o = offered.get(cardId) ?? 0;
			const p = picked.get(cardId) ?? 0;
			const m = meta.get(cardId) ?? { category: '?', rarity: '?' };
			return { cardId, category: m.category, rarity: m.rarity, offered: o, picked: p, pickRate: o ? p / o : 0 };
		})
		.sort((a, b) => b.pickRate - a.pickRate || b.offered - a.offered);
}

/* ── 목록 테이블 ─────────────────────────────────────────── */

export interface RunRow {
	runId: string;
	startedAtUtc: string;
	weapon: string;
	result: string;
	durationSec: number;
	fights: number;
	bestBossHpPct: number;
	parryRate: number;
	totalDamageTaken: number;
	cards: number;
	run: RunLog;
}

export function runRows(runs: RunLog[]): RunRow[] {
	return runs.map((run) => {
		const fights = run.bossFights;
		const pa = fights.reduce((s, f) => s + f.parry.attempts, 0);
		const ps = fights.reduce((s, f) => s + f.parry.successes, 0);
		return {
			runId: run.runId,
			startedAtUtc: run.startedAtUtc,
			weapon: run.weapon,
			result: run.result,
			durationSec: run.durationSec,
			fights: fights.length,
			bestBossHpPct: fights.length ? Math.min(...fights.map((f) => f.bossHealthPctAtEnd)) : 1,
			parryRate: pa ? ps / pa : 0,
			totalDamageTaken: fights.reduce((s, f) => s + f.totalDamageTaken, 0),
			cards: run.cardsPicked.length,
			run,
		};
	});
}
