// UE 측(ACWebDebugTypes.h)과 공유하는 JSON 스키마 타입 정의 — 이 파일이 계약이다.
// 필드를 바꾸면 반드시 Source/Ashen_Cathedral/Public/Debug/ACWebDebugTypes.h 도 함께 바꿔야 한다.

export const SCHEMA_VERSION = 1;

/* ────────────────────────────── 타임라인 ────────────────────────────── */

/** WebSocket 연결 직후 1회, 전투 시작 시 갱신되어 전송되는 세션 헤더 */
export interface SessionHeader {
	kind: 'session';
	schema: number;
	sessionId: string;
	runSeed: number;
	bossId: string;
	attempt: number;
	weapon: string;
	buildConfig: string;
	startedAtUtc: string;
}

export type EventType = 'gauge' | 'ability' | 'tag' | 'hit' | 'marker';
export type EventSource = 'player' | 'boss';
export type EventPhase = 'begin' | 'end' | 'instant';

/** gauge 이벤트로 기록되는 게이지 — 이 5개만 기록한다 */
export const GAUGE_KEYS = ['Health', 'Stamina', 'Posture', 'GuardGauge', 'BurnGauge'] as const;
export type GaugeKey = (typeof GAUGE_KEYS)[number];

/** marker 이벤트의 key 허용값 */
export const MARKER_KEYS = [
	'GuardBroken',
	'PostureBroken',
	'ParrySuccess',
	'ParryFail',
	'BlockSuccess',
	'CriticalAttack',
	'Phase2Enter',
	'Death',
	'BossDeath',
] as const;
export type MarkerKey = (typeof MARKER_KEYS)[number];

/** hit 이벤트에만 붙는 부가 정보 */
export interface HitMeta {
	attackTags: string[];
	sourceAbility: string;
	direction: string;
	postureDamage: number;
	guardDamage: number;
	wasBlocked: boolean;
	wasParried: boolean;
}

export interface TimelineEvent {
	/** 전투 시작 기준 경과 초 (소수 3자리) */
	t: number;
	type: EventType;
	src: EventSource;
	/** 어트리뷰트명 / 어빌리티 클래스명 / 태그 전체 경로 / 마커명 */
	key: string;
	/** gauge 전용 — 0.0~1.0 정규화 값. 차트는 이 값만 쓴다 */
	norm?: number;
	/** 절대값 (툴팁 표시용) */
	raw?: number;
	/** ability / tag 전용 */
	phase?: EventPhase;
	/** hit 전용 */
	meta?: HitMeta;
}

export interface EventBatch {
	kind: 'events';
	events: TimelineEvent[];
}

export type WebDebugMessage = SessionHeader | EventBatch;

/** 사망 시 게임이 저장하는 스냅샷 파일 형식 (드래그 앤 드롭으로 불러온다) */
export interface TimelineSnapshot {
	schema: number;
	session: SessionHeader;
	events: TimelineEvent[];
}

export function isSessionHeader(m: WebDebugMessage): m is SessionHeader {
	return m.kind === 'session';
}

export function isEventBatch(m: WebDebugMessage): m is EventBatch {
	return m.kind === 'events';
}

/* ────────────────────────────── 런 로그 ────────────────────────────── */

export type RunResult = 'died' | 'cleared' | 'abandoned';
export type BossFightResult = 'won' | 'lost';

export interface AttemptStat {
	attempts: number;
	successes: number;
}

export interface DodgeStat {
	attempts: number;
	iframeSuccesses: number;
}

export interface DeathCause {
	attackTag: string;
	bossAbility: string;
	playerHealthPctBefore: number;
	damage: number;
}

/**
 * 이 도구의 핵심 가치. 세 배열 모두 그 보스전 동안의 전체 표본을 담는다.
 * - parryInputOffsetMs      : 패링 판정 윈도우 시작 대비 입력 시각. 음수 = 일찍 누름
 * - hitAfterIframeEndMs     : 무적 종료 대비 피격 시각. 양수 = 무적 끝난 뒤
 * - comboWindowInputOffsetMs: 콤보 윈도우 시작 대비 입력 시각. 음수 = 일찍 누름
 */
export interface TimingSamples {
	parryInputOffsetMs: number[];
	hitAfterIframeEndMs: number[];
	comboWindowInputOffsetMs: number[];
}

export interface BossFight {
	bossId: string;
	attempt: number;
	result: BossFightResult;
	durationSec: number;
	bossHealthPctAtEnd: number;

	parry: AttemptStat;
	dodge: DodgeStat;
	block: AttemptStat;

	guardBreaksTaken: number;
	guardBreaksInflicted: number;
	postureBreaksTaken: number;
	postureBreaksInflicted: number;
	criticalAttacksLanded: number;
	totalDamageDealt: number;
	totalDamageTaken: number;
	avgPlayerHealthPct: number;

	damageTakenByAttackTag: Record<string, number>;
	deathCause?: DeathCause;
	timingSamples: TimingSamples;
}

export interface CardPick {
	cardId: string;
	rarity: string;
	category: string;
	stackAfter: number;
	/** 함께 제시되었으나 선택되지 않은 카드 ID들 — 픽률 계산의 분모 */
	offeredWith: string[];
	afterBossId: string;
	pickIndex: number;
}

export interface RunLog {
	schema: number;
	runId: string;
	seed: number;
	startedAtUtc: string;
	durationSec: number;
	weapon: string;
	result: RunResult;

	metaUpgrades: Record<string, number>;
	currencyEarned: Record<string, number>;
	cardsPicked: CardPick[];
	bossFights: BossFight[];
}

/* ────────────────────────────── 표시용 헬퍼 ────────────────────────────── */

/** "MetaProgression.BossID.AshenKnight" → "AshenKnight" */
export function shortTag(tag: string): string {
	const i = tag.lastIndexOf('.');
	return i < 0 ? tag : tag.slice(i + 1);
}

/** "Player.Weapon.Katana" → "Katana" */
export const shortWeapon = shortTag;
