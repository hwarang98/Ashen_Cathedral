// 대시보드 색상 팔레트 — 다크가 기본이며 라이트는 자동 반전이 아니라 별도로 고른 값이다.
//
// 계열 색은 순위가 아니라 대상을 따라간다. GAUGE_SERIES 의 순서와 색은 고정이며,
// 사용자가 특정 레인을 꺼도 남은 계열의 색은 절대 바뀌지 않는다.
//
// 검증(scripts/validate-palette.mjs, dataviz 검증기와 동일한 계산):
//   dark  surface #17171b — CVD 인접쌍 최저 ΔE 8.4 (protan), 일반시야 최저 19.8 → PASS
//   light surface #fbfbf9 — CVD 인접쌍 최저 ΔE 9.1 (protan), 일반시야 최저 22.9 → PASS
// 계열 순서(Health→GuardGauge→Posture→Stamina→BurnGauge)가 곧 CVD 안전장치이므로
// 색을 바꾸거나 순서를 바꿀 때는 반드시 npm run validate:palette 를 다시 돌릴 것.

import type { GaugeKey, MarkerKey } from './schema';

export interface SeriesSpec {
	key: GaugeKey;
	label: string;
	dark: string;
	light: string;
	/** 색 외 2차 인코딩 — canvas 선의 dash 패턴 (CVD 대비) */
	dash: number[];
}

/** 고정 순서. 인덱스가 곧 색 슬롯이다. */
export const GAUGE_SERIES: SeriesSpec[] = [
	{ key: 'Health', label: 'Health', dark: '#e66767', light: '#e34948', dash: [] },
	{ key: 'GuardGauge', label: 'Guard', dark: '#3987e5', light: '#2a78d6', dash: [7, 4] },
	{ key: 'Posture', label: 'Posture', dark: '#c98500', light: '#eda100', dash: [2, 3] },
	{ key: 'Stamina', label: 'Stamina', dark: '#199e70', light: '#1baf7a', dash: [11, 4, 2, 4] },
	{ key: 'BurnGauge', label: 'Burn', dark: '#d95926', light: '#eb6834', dash: [4, 3, 1, 3] },
];

const SERIES_BY_KEY = new Map(GAUGE_SERIES.map((s) => [s.key, s]));

export type ThemeMode = 'dark' | 'light';

export function seriesColor(key: GaugeKey, mode: ThemeMode): string {
	const spec = SERIES_BY_KEY.get(key);
	if (!spec) return mode === 'dark' ? '#898781' : '#898781';
	return mode === 'dark' ? spec.dark : spec.light;
}

export function seriesSpec(key: GaugeKey): SeriesSpec | undefined {
	return SERIES_BY_KEY.get(key);
}

/** 상태색 — 계열 색과 절대 겹쳐 쓰지 않는다. 항상 라벨/아이콘과 함께 쓴다. */
export const STATUS = {
	good: '#0ca30c',
	warning: '#fab219',
	serious: '#ec835a',
	critical: '#d03b3b',
} as const;

export interface ChromeTokens {
	surface: string;
	plane: string;
	textPrimary: string;
	textSecondary: string;
	textMuted: string;
	grid: string;
	axis: string;
	border: string;
	/** 플레이어/보스 구분용 중립 톤 — 간트 막대 배경 */
	lanePlayer: string;
	laneBoss: string;
}

export const CHROME: Record<ThemeMode, ChromeTokens> = {
	dark: {
		surface: '#17171b',
		plane: '#0e0e11',
		textPrimary: '#ffffff',
		textSecondary: '#c3c2b7',
		textMuted: '#898781',
		grid: '#26262b',
		axis: '#3a3a40',
		border: 'rgba(255,255,255,0.10)',
		lanePlayer: '#2b2b33',
		laneBoss: '#332b2b',
	},
	light: {
		surface: '#fbfbf9',
		plane: '#f2f2ee',
		textPrimary: '#0b0b0b',
		textSecondary: '#52514e',
		textMuted: '#898781',
		grid: '#e1e0d9',
		axis: '#c3c2b7',
		border: 'rgba(11,11,11,0.10)',
		lanePlayer: '#e6e6ea',
		laneBoss: '#eee0dd',
	},
};

/**
 * 마커 표시 규격 — 색만으로 구분하지 않도록 글리프(형태)를 함께 쓴다.
 * 상태색은 4종뿐이므로 여러 마커가 같은 색을 공유하며, 형태와 라벨이 정체성을 담당한다.
 */
export const MARKER_STYLE: Record<MarkerKey, { color: string; glyph: string; label: string }> = {
	GuardBroken: { color: STATUS.serious, glyph: '▽', label: '가드 브레이크' },
	PostureBroken: { color: STATUS.warning, glyph: '◇', label: '체간 붕괴' },
	ParrySuccess: { color: STATUS.good, glyph: '△', label: '패링 성공' },
	ParryFail: { color: STATUS.critical, glyph: '▲', label: '패링 실패' },
	BlockSuccess: { color: STATUS.good, glyph: '□', label: '블록 성공' },
	CriticalAttack: { color: STATUS.warning, glyph: '★', label: '치명타' },
	Phase2Enter: { color: STATUS.serious, glyph: '◈', label: 'Phase 2' },
	Death: { color: STATUS.critical, glyph: '✕', label: '사망' },
	BossDeath: { color: STATUS.good, glyph: '✦', label: '보스 격파' },
};

/** hit 이벤트 표시색 — 마커 레인의 피격 표식 */
export const HIT_COLOR = STATUS.critical;

/** 태그 레인 막대 색 — 판정 성격에 따라 상태색을 쓴다(계열 색과 분리) */
export function tagBarColor(tag: string): string {
	if (tag.includes('Invincible')) return STATUS.good;
	if (tag.includes('Parry')) return STATUS.warning;
	if (tag.includes('SuperArmor')) return STATUS.serious;
	if (tag.includes('ComboWindow') || tag.includes('SpecialLinkWindow')) return '#3987e5';
	if (tag.includes('Broken') || tag.includes('Stagger') || tag.includes('Dead')) return STATUS.critical;
	return '#898781';
}

/** 현재 문서 테마를 읽는다 — data-theme 스탬프가 OS 설정을 이긴다 */
export function currentMode(): ThemeMode {
	const stamped = document.documentElement.getAttribute('data-theme');
	if (stamped === 'light' || stamped === 'dark') return stamped;
	return window.matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark';
}
