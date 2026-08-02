// mock 데이터 생성기 — 언리얼 없이 대시보드를 개발/검증하기 위한 재현 가능한 데이터.
//   npm run gen:mock
// 출력: mock/sample-timeline.json, mock/runs/run-0001..0020.json, mock/runs/index.json
//
// 런 20개는 "학습 곡선이 보이도록" 만든다.
//   - bossHealthPctAtEnd 가 시도 횟수에 따라 점진적으로 낮아진다
//   - parry.successes / attempts 비율이 올라간다
//   - parryInputOffsetMs 분포는 계속 음수 쪽에 치우쳐 있다 (= 패링 윈도우를 앞으로 옮겨야 한다는 신호)
//   - hitAfterIframeEndMs 는 0~50ms 구간에 몰려 있다 (= 무적 프레임이 짧다는 신호)

import { mkdirSync, writeFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const MOCK = resolve(ROOT, 'mock');
const RUNS = resolve(MOCK, 'runs');

/* ── 결정적 난수 ─────────────────────────────────────────────── */
function mulberry32(seed) {
	let a = seed >>> 0;
	return () => {
		a = (a + 0x6d2b79f5) >>> 0;
		let t = Math.imul(a ^ (a >>> 15), 1 | a);
		t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
		return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
	};
}
const round = (v, d = 3) => Number(v.toFixed(d));
/** 평균 mu, 표준편차 sigma 의 정규분포 표본 */
function gauss(rnd, mu, sigma) {
	const u = Math.max(rnd(), 1e-9);
	const v = rnd();
	return mu + sigma * Math.sqrt(-2 * Math.log(u)) * Math.cos(2 * Math.PI * v);
}

/* ── 타임라인 ─────────────────────────────────────────────────── */

const BOSS_ABILITIES = [
	{ name: 'GA_Enemy_Attack_Combo_01', dur: 1.1, tag: 'Enemy.Ability.Melee', dmg: 96, posture: 18, blockable: true, parryable: true },
	{ name: 'GA_Enemy_Attack_Combo_02', dur: 1.5, tag: 'Enemy.Ability.Melee', dmg: 128, posture: 24, blockable: true, parryable: true },
	{ name: 'GA_Enemy_Attack_Run', dur: 1.3, tag: 'Enemy.Ability.AttackType.Run', dmg: 112, posture: 22, blockable: true, parryable: false },
	{ name: 'GA_Enemy_Attack_Special_01', dur: 2.0, tag: 'Enemy.Ability.AttackType.Special_01', dmg: 186, posture: 40, blockable: false, parryable: false },
	{ name: 'GA_Enemy_Attack_Special_02', dur: 1.8, tag: 'Enemy.Ability.AttackType.Special_02', dmg: 154, posture: 34, blockable: true, parryable: true },
];

const PLAYER_ATTACKS = [
	{ name: 'GA_Player_Attack_Light_01', dur: 0.55, wind: 0.1, dmg: 84, stamina: 12 },
	{ name: 'GA_Player_Attack_Light_02', dur: 0.6, wind: 0.1, dmg: 92, stamina: 13 },
	{ name: 'GA_Player_Attack_Heavy_01', dur: 1.15, wind: 0.4, dmg: 176, stamina: 26 },
];

function buildTimeline() {
	const rnd = mulberry32(8291);
	const events = [];
	const DURATION = 92.0;
	const TICK = 1 / 15;

	// 게이지 상태 (0~1 정규화 전 절대값)
	const P = { Health: 1000, MaxHealth: 1000, Stamina: 160, MaxStamina: 160, Posture: 0, MaxPosture: 120, GuardGauge: 0, MaxGuardGauge: 90, BurnGauge: 0, MaxBurnGauge: 100 };
	const B = { Health: 5200, MaxHealth: 5200, Posture: 0, MaxPosture: 480 };

	// 값이 변했을 때만 기록하기 위한 마지막 전송값
	const lastSent = new Map();
	function gauge(t, src, key, cur, max) {
		const norm = max > 0 ? Math.min(1, Math.max(0, cur / max)) : 0;
		const id = `${src}:${key}`;
		const prev = lastSent.get(id);
		const rounded = round(norm, 3);
		if (prev === rounded) return;
		lastSent.set(id, rounded);
		events.push({ t: round(t), type: 'gauge', src, key, norm: rounded, raw: round(cur, 1) });
	}
	const span = (t0, t1, src, key, type = 'tag') => {
		events.push({ t: round(t0), type, src, key, phase: 'begin' });
		events.push({ t: round(t1), type, src, key, phase: 'end' });
	};
	const marker = (t, src, key) => events.push({ t: round(t), type: 'marker', src, key });

	// 스케줄된 이산 사건들을 시간순으로 미리 만든다
	const schedule = [];
	// 샘플링 루프가 돌기 시작한 뒤 추가되는 사건은 정렬 위치에 끼워 넣어야 소비 순서가 깨지지 않는다
	const schedulePush = (item) => {
		let i = schedule.length;
		while (i > 0 && schedule[i - 1].t > item.t) i--;
		schedule.splice(i, 0, item);
	};
	let t = 1.2;
	let comboIndex = 0;
	let phase2Fired = false;
	let deathT = null;

	while (t < DURATION && deathT === null) {
		// ── 보스 공격 1회
		const atk = BOSS_ABILITIES[Math.floor(rnd() * BOSS_ABILITIES.length)];
		const abStart = t;
		const abEnd = t + atk.dur;
		const impact = t + atk.dur * 0.62;
		span(abStart, abEnd, 'boss', atk.name, 'ability');
		if (atk.name.includes('Special')) span(abStart + 0.1, abEnd - 0.1, 'boss', 'Shared.Status.SuperArmor');

		// ── 플레이어 반응 선택
		const roll = rnd();
		let outcome;
		if (atk.parryable && roll < 0.22) outcome = 'parry';
		else if (roll < 0.44) outcome = 'dodge';
		else if (atk.blockable && roll < 0.76) outcome = 'block';
		else outcome = 'hit';

		if (outcome === 'parry') {
			// 패링 판정 윈도우 0.25s — 입력은 대체로 조금 이르다
			const inputOffset = gauss(rnd, -0.07, 0.07);
			const windowStart = impact - 0.16 + inputOffset;
			span(windowStart, windowStart + 0.25, 'player', 'Shared.Status.Parry');
			span(windowStart - 0.02, windowStart + 0.45, 'player', 'GA_Player_Parry', 'ability');
			const success = impact >= windowStart && impact <= windowStart + 0.25;
			marker(impact, 'player', success ? 'ParrySuccess' : 'ParryFail');
			if (success) {
				schedule.push({ t: impact, fn: () => { B.Posture = Math.min(B.MaxPosture, B.Posture + 70); } });
				span(impact, impact + 0.8, 'boss', 'Shared.Status.Stagger');
			} else {
				outcome = 'hit';
			}
		}

		if (outcome === 'dodge') {
			// 회피: 무적 0.20s
			const iStart = impact - 0.09 + gauss(rnd, 0, 0.06);
			span(iStart, iStart + 0.2, 'player', 'Shared.Status.Invincible');
			span(iStart - 0.05, iStart + 0.55, 'player', 'GA_Player_Roll', 'ability');
			span(iStart - 0.05, iStart + 0.55, 'player', 'Player.Status.Rolling');
			schedule.push({ t: iStart, fn: () => { P.Stamina = Math.max(0, P.Stamina - 24); } });
			const evaded = impact >= iStart && impact <= iStart + 0.2;
			if (!evaded) outcome = 'hit';
		}

		if (outcome === 'block') {
			span(impact - 0.35, impact + 0.5, 'player', 'Player.Status.Blocking');
			span(impact - 0.35, impact + 0.5, 'player', 'GA_Player_Block', 'ability');
			marker(impact, 'player', 'BlockSuccess');
			const gd = atk.posture * 1.15;
			schedule.push({
				t: impact,
				fn: () => {
					P.GuardGauge += gd;
					P.Stamina = Math.max(0, P.Stamina - 14);
					P.Posture = Math.min(P.MaxPosture, P.Posture + atk.posture * 0.4);
					P.Health = Math.max(0, P.Health - atk.dmg * 0.15);
					events.push({
						t: round(impact), type: 'hit', src: 'player', key: 'damage_taken', raw: round(atk.dmg * 0.15, 1),
						meta: {
							attackTags: [atk.blockable ? 'Shared.Attack.Blockable' : 'Shared.Attack.Unblockable', ...(atk.posture > 30 ? ['Shared.Attack.Weight.Heavy'] : [])],
							sourceAbility: atk.name, direction: 'Front',
							postureDamage: 0, guardDamage: round(gd, 1), wasBlocked: true, wasParried: false,
						},
					});
					if (P.GuardGauge >= P.MaxGuardGauge) {
						P.GuardGauge = 0;
						marker(impact + 0.01, 'player', 'GuardBroken');
						span(impact + 0.01, impact + 1.4, 'player', 'Player.Status.GuardBroken');
					}
				},
			});
		}

		if (outcome === 'hit') {
			const dirs = ['Front', 'Front', 'Left', 'Right', 'Back'];
			const dir = dirs[Math.floor(rnd() * dirs.length)];
			schedule.push({
				t: impact,
				fn: () => {
					P.Health = Math.max(0, P.Health - atk.dmg);
					P.Posture = Math.min(P.MaxPosture, P.Posture + atk.posture);
					if (phase2Fired) P.BurnGauge = Math.min(P.MaxBurnGauge, P.BurnGauge + 18);
					events.push({
						t: round(impact), type: 'hit', src: 'player', key: 'damage_taken', raw: round(atk.dmg, 1),
						meta: {
							attackTags: [
								atk.blockable ? 'Shared.Attack.Blockable' : 'Shared.Attack.Unblockable',
								atk.parryable ? 'Shared.Attack.Parryable' : 'Shared.Attack.Unparryable',
								...(atk.posture > 30 ? ['Shared.Attack.Weight.Heavy'] : []),
							],
							sourceAbility: atk.name, direction: dir,
							postureDamage: round(atk.posture, 1), guardDamage: 0, wasBlocked: false, wasParried: false,
						},
					});
					span(impact, impact + 0.55, 'player', 'Shared.Status.HitReact');
					span(impact, impact + 0.55, 'player', `Shared.Status.HitReact.${dir}`);
					if (P.Posture >= P.MaxPosture) {
						P.Posture = 0;
						marker(impact + 0.01, 'player', 'PostureBroken');
						span(impact + 0.01, impact + 2.2, 'player', 'Shared.Status.PostureBroken');
					}
					if (P.Health <= 0) deathT = impact + 0.05;
				},
			});
		}

		t = abEnd + 0.15;

		// ── 플레이어 반격 콤보 (보스 공격 사이)
		const comboCount = 1 + Math.floor(rnd() * 3);
		for (let c = 0; c < comboCount && t < DURATION; c++) {
			const pa = PLAYER_ATTACKS[comboIndex++ % PLAYER_ATTACKS.length];
			const s = t;
			const e = t + pa.dur;
			span(s, e, 'player', pa.name, 'ability');
			span(s, e, 'player', 'Player.ActionState.Attacking');
			// 콤보 윈도우는 후반부에 열린다
			span(s + pa.dur * 0.55, e, 'player', 'Player.Status.ComboWindow');
			const land = s + pa.wind + 0.08;
			schedule.push({
				t: land,
				fn: () => {
					P.Stamina = Math.max(0, P.Stamina - pa.stamina);
					B.Health = Math.max(0, B.Health - pa.dmg);
					B.Posture = Math.min(B.MaxPosture, B.Posture + pa.dmg * 0.22);
					events.push({
						t: round(land), type: 'hit', src: 'boss', key: 'damage_taken', raw: round(pa.dmg, 1),
						meta: {
							attackTags: ['Shared.Attack.Blockable'], sourceAbility: pa.name, direction: 'Front',
							postureDamage: round(pa.dmg * 0.22, 1), guardDamage: 0, wasBlocked: false, wasParried: false,
						},
					});
					if (B.Posture >= B.MaxPosture) {
						B.Posture = 0;
						marker(land + 0.01, 'boss', 'PostureBroken');
						span(land + 0.01, land + 3.0, 'boss', 'Shared.Status.PostureBroken');
						marker(land + 0.6, 'player', 'CriticalAttack');
						schedulePush({ t: land + 0.6, fn: () => { B.Health = Math.max(0, B.Health - 900); } });
						span(land + 0.5, land + 2.6, 'player', 'GA_Player_CriticalAttack', 'ability');
						span(land + 0.5, land + 2.6, 'player', 'Shared.Status.Invincible');
					}
				},
			});
			t = e + 0.05;
		}
		t += 0.35 + rnd() * 0.6;
	}

	schedule.sort((a, b) => a.t - b.t);

	// ── 15 Hz 샘플링 루프: 스케줄된 사건을 시간순으로 소비하며 게이지를 스냅샷
	let si = 0;
	const endT = deathT !== null ? deathT + 1.5 : DURATION;
	for (let ts = 0; ts <= endT + 1e-6; ts += TICK) {
		while (si < schedule.length && schedule[si].t <= ts) schedule[si++].fn();

		// 자연 회복/감소
		P.Stamina = Math.min(P.MaxStamina, P.Stamina + 26 * TICK);
		P.Posture = Math.max(0, P.Posture - 1.4 * TICK);
		P.GuardGauge = Math.max(0, P.GuardGauge - 1.5 * TICK);
		P.BurnGauge = Math.max(0, P.BurnGauge - 3 * TICK);
		B.Posture = Math.max(0, B.Posture - 22 * TICK);

		// Phase 2 — 보스 HP 50% 이하
		if (!phase2Fired && B.Health / B.MaxHealth <= 0.5) {
			phase2Fired = true;
			marker(ts, 'boss', 'Phase2Enter');
			events.push({ t: round(ts), type: 'tag', src: 'boss', key: 'Enemy.Status.Phase2', phase: 'begin' });
		}

		gauge(ts, 'player', 'Health', P.Health, P.MaxHealth);
		gauge(ts, 'player', 'Stamina', P.Stamina, P.MaxStamina);
		gauge(ts, 'player', 'Posture', P.Posture, P.MaxPosture);
		gauge(ts, 'player', 'GuardGauge', P.GuardGauge, P.MaxGuardGauge);
		gauge(ts, 'player', 'BurnGauge', P.BurnGauge, P.MaxBurnGauge);
		gauge(ts, 'boss', 'Health', B.Health, B.MaxHealth);
		gauge(ts, 'boss', 'Posture', B.Posture, B.MaxPosture);

		if (deathT !== null && ts >= deathT) break;
	}

	if (deathT !== null) {
		marker(deathT, 'player', 'Death');
		events.push({ t: round(deathT), type: 'tag', src: 'player', key: 'Shared.Status.Dead', phase: 'begin' });
		events.push({ t: round(deathT + 1.2), type: 'tag', src: 'player', key: 'Shared.Status.Dead', phase: 'end' });
	}

	events.sort((a, b) => a.t - b.t);

	return {
		schema: 1,
		session: {
			kind: 'session',
			schema: 1,
			sessionId: 'a3f2c891',
			runSeed: 8291,
			bossId: 'MetaProgression.BossID.AshenKnight',
			attempt: 12,
			weapon: 'Player.Weapon.Katana',
			buildConfig: 'Development',
			startedAtUtc: '2026-08-01T13:00:00Z',
		},
		events,
	};
}

/* ── 런 로그 20개 ─────────────────────────────────────────────── */

const CARD_POOL = [
	{ id: 'A01', category: 'Attack', rarity: 'Common', appeal: 0.9 },
	{ id: 'A02', category: 'Attack', rarity: 'Uncommon', appeal: 0.75 },
	{ id: 'A03', category: 'Attack', rarity: 'Rare', appeal: 0.95 },
	{ id: 'A04', category: 'Attack', rarity: 'Common', appeal: 0.12 },
	{ id: 'D01', category: 'Defense', rarity: 'Common', appeal: 0.55 },
	{ id: 'D02', category: 'Defense', rarity: 'Uncommon', appeal: 0.4 },
	{ id: 'D03', category: 'Defense', rarity: 'Rare', appeal: 0.8 },
	{ id: 'M01', category: 'Mobility', rarity: 'Common', appeal: 0.7 },
	{ id: 'M02', category: 'Mobility', rarity: 'Uncommon', appeal: 0.5 },
	{ id: 'P01', category: 'Parry', rarity: 'Common', appeal: 0.6 },
	{ id: 'P02', category: 'Parry', rarity: 'Rare', appeal: 0.85 },
	{ id: 'P03', category: 'Parry', rarity: 'Legendary', appeal: 1.0 },
	{ id: 'R01', category: 'Resource', rarity: 'Common', appeal: 0.35 },
	{ id: 'R02', category: 'Resource', rarity: 'Uncommon', appeal: 0.0 }, // 픽률 0% — 존재하지 않는 것과 같은 카드
];

const ATTACK_TAGS = [
	'Enemy.Ability.AttackType.Special_01',
	'Enemy.Ability.Melee',
	'Enemy.Ability.AttackType.Run',
	'Enemy.Ability.AttackType.Special_02',
	'Enemy.Ability.AttackType.BackDash',
];

const WEAPONS = ['Player.Weapon.Katana', 'Player.Weapon.Sword', 'Player.Weapon.Nodachi'];

function buildRun(index) {
	const attempt = index; // 1-based
	const rnd = mulberry32(100000 + index * 977);
	// 학습 진행도 0 → 1
	const skill = Math.min(1, (index - 1) / 17);
	const noise = () => (rnd() - 0.5) * 0.09;

	const clearedKnight = skill > 0.72 && rnd() > 0.25;
	const knightHpEnd = clearedKnight ? 0 : Math.max(0.02, 0.92 - skill * 0.86 + noise());

	const parryAttempts = 12 + Math.floor(rnd() * 18) + Math.floor(skill * 10);
	const parryRate = 0.16 + skill * 0.46 + noise() * 0.5;
	const parrySuccesses = Math.min(parryAttempts, Math.max(0, Math.round(parryAttempts * parryRate)));

	const dodgeAttempts = 26 + Math.floor(rnd() * 22);
	const dodgeRate = 0.22 + skill * 0.34 + noise() * 0.4;
	const dodgeSuccesses = Math.max(0, Math.round(dodgeAttempts * dodgeRate));

	const blockAttempts = 18 + Math.floor(rnd() * 22);
	const blockSuccesses = Math.max(0, Math.round(blockAttempts * (0.82 + skill * 0.12 + noise() * 0.2)));

	// 패링 입력 오프셋: 계속 음수 쪽에 치우쳐 있고, 학습으로 분산만 줄어든다
	const parryMu = -128 + skill * 78;
	const parrySigma = 96 - skill * 44;
	const parryInputOffsetMs = Array.from({ length: parryAttempts }, () => Math.round(gauss(rnd, parryMu, parrySigma)));

	// 무적 종료 후 피격: 0~50ms 에 몰린다 (무적 프레임이 짧다는 신호)
	const iframeCount = 6 + Math.floor(rnd() * 10);
	const hitAfterIframeEndMs = Array.from({ length: iframeCount }, () => {
		const near = rnd() < 0.66;
		return Math.max(0, Math.round(near ? gauss(rnd, 24, 16) : gauss(rnd, 260, 150)));
	});

	const comboCount = 8 + Math.floor(rnd() * 10);
	const comboInputOffsetMs = Array.from({ length: comboCount }, () =>
		Math.round(gauss(rnd, -18 + skill * 22, 52 - skill * 18))
	);

	const totalDamageTaken = Math.round(1600 - skill * 620 + rnd() * 300);
	const damageTakenByAttackTag = {};
	let remaining = totalDamageTaken;
	ATTACK_TAGS.forEach((tag, i) => {
		const share = i === ATTACK_TAGS.length - 1 ? remaining : Math.round(remaining * (0.42 - i * 0.06 + rnd() * 0.1));
		damageTakenByAttackTag[tag] = Math.max(0, share);
		remaining -= share;
	});

	const deathTag = ATTACK_TAGS[rnd() < 0.5 ? 0 : Math.floor(rnd() * ATTACK_TAGS.length)];
	const knightFight = {
		bossId: 'MetaProgression.BossID.AshenKnight',
		attempt,
		result: clearedKnight ? 'won' : 'lost',
		durationSec: round(88 + skill * 92 + rnd() * 40, 1),
		bossHealthPctAtEnd: round(knightHpEnd, 3),
		parry: { attempts: parryAttempts, successes: parrySuccesses },
		dodge: { attempts: dodgeAttempts, iframeSuccesses: dodgeSuccesses },
		block: { attempts: blockAttempts, successes: blockSuccesses },
		guardBreaksTaken: Math.max(0, Math.round(4 - skill * 3 + rnd() * 2)),
		guardBreaksInflicted: Math.round(skill * 3 + rnd()),
		postureBreaksTaken: Math.max(0, Math.round(2 - skill * 2 + rnd())),
		postureBreaksInflicted: Math.round(skill * 3 + rnd() * 2),
		criticalAttacksLanded: Math.round(skill * 3 + rnd() * 2),
		totalDamageDealt: Math.round(2400 + skill * 7200 + rnd() * 900),
		totalDamageTaken,
		avgPlayerHealthPct: round(0.38 + skill * 0.3 + noise(), 3),
		damageTakenByAttackTag,
		timingSamples: {
			parryInputOffsetMs,
			hitAfterIframeEndMs,
			comboWindowInputOffsetMs: comboInputOffsetMs,
		},
	};
	if (!clearedKnight) {
		knightFight.deathCause = {
			attackTag: deathTag,
			bossAbility: `GA_Enemy_Attack_${deathTag.split('.').pop()}`,
			playerHealthPctBefore: round(0.08 + rnd() * 0.22, 3),
			damage: round(120 + rnd() * 160, 1),
		};
	}

	const bossFights = [knightFight];

	// 재검(AshenKnight)을 넘긴 런은 Ordan 까지 간다
	if (clearedKnight) {
		const oSkill = Math.max(0, skill - 0.72) / 0.28;
		const oCleared = oSkill > 0.7 && rnd() > 0.5;
		const oParryAttempts = 14 + Math.floor(rnd() * 12);
		const oParrySucc = Math.round(oParryAttempts * (0.2 + oSkill * 0.3));
		const oDamageTaken = Math.round(1300 - oSkill * 300 + rnd() * 260);
		const oByTag = {};
		let rem = oDamageTaken;
		ATTACK_TAGS.forEach((tag, i) => {
			const share = i === ATTACK_TAGS.length - 1 ? rem : Math.round(rem * (0.38 - i * 0.05 + rnd() * 0.1));
			oByTag[tag] = Math.max(0, share);
			rem -= share;
		});
		const oFight = {
			bossId: 'MetaProgression.BossID.Ordan',
			attempt: Math.max(1, index - 13),
			result: oCleared ? 'won' : 'lost',
			durationSec: round(70 + oSkill * 60 + rnd() * 30, 1),
			bossHealthPctAtEnd: oCleared ? 0 : round(Math.max(0.05, 0.8 - oSkill * 0.6 + noise()), 3),
			parry: { attempts: oParryAttempts, successes: oParrySucc },
			dodge: { attempts: 22 + Math.floor(rnd() * 16), iframeSuccesses: Math.round((22 + rnd() * 16) * (0.3 + oSkill * 0.25)) },
			block: { attempts: 16 + Math.floor(rnd() * 14), successes: Math.round((16 + rnd() * 14) * 0.85) },
			guardBreaksTaken: Math.round(3 - oSkill * 2 + rnd()),
			guardBreaksInflicted: Math.round(oSkill * 2),
			postureBreaksTaken: Math.round(2 - oSkill + rnd()),
			postureBreaksInflicted: Math.round(oSkill * 2 + rnd()),
			criticalAttacksLanded: Math.round(oSkill * 2 + rnd()),
			totalDamageDealt: Math.round(3000 + oSkill * 5200 + rnd() * 700),
			totalDamageTaken: oDamageTaken,
			avgPlayerHealthPct: round(0.42 + oSkill * 0.2 + noise(), 3),
			damageTakenByAttackTag: oByTag,
			timingSamples: {
				parryInputOffsetMs: Array.from({ length: oParryAttempts }, () => Math.round(gauss(rnd, -142 + oSkill * 60, 104 - oSkill * 30))),
				hitAfterIframeEndMs: Array.from({ length: 5 + Math.floor(rnd() * 8) }, () =>
					Math.max(0, Math.round(rnd() < 0.7 ? gauss(rnd, 28, 18) : gauss(rnd, 280, 160)))
				),
				comboWindowInputOffsetMs: Array.from({ length: 6 + Math.floor(rnd() * 8) }, () => Math.round(gauss(rnd, -10, 48))),
			},
		};
		if (!oCleared) {
			const oDeathTag = ATTACK_TAGS[Math.floor(rnd() * ATTACK_TAGS.length)];
			oFight.deathCause = {
				attackTag: oDeathTag,
				bossAbility: `GA_Ordan_Attack_${oDeathTag.split('.').pop()}`,
				playerHealthPctBefore: round(0.06 + rnd() * 0.2, 3),
				damage: round(140 + rnd() * 180, 1),
			};
		}
		bossFights.push(oFight);
	}

	// 카드 픽 — 보스를 하나 깰 때마다 3장 중 1장
	const cardsPicked = [];
	let pickIndex = 0;
	for (const fight of bossFights) {
		if (fight.result !== 'won') continue;
		const offered = [];
		const poolCopy = [...CARD_POOL];
		for (let i = 0; i < 3 && poolCopy.length; i++) {
			offered.push(poolCopy.splice(Math.floor(rnd() * poolCopy.length), 1)[0]);
		}
		// 매력도 가중 선택 — 매력도 0인 카드는 절대 뽑히지 않는다
		const totalAppeal = offered.reduce((s, c) => s + c.appeal, 0);
		let r = rnd() * (totalAppeal || 1);
		let chosen = offered[0];
		for (const c of offered) {
			r -= c.appeal;
			if (r <= 0) { chosen = c; break; }
		}
		const stackAfter = cardsPicked.filter((c) => c.cardId === chosen.id).length + 1;
		cardsPicked.push({
			cardId: chosen.id,
			rarity: chosen.rarity,
			category: chosen.category,
			stackAfter,
			offeredWith: offered.filter((c) => c.id !== chosen.id).map((c) => c.id),
			afterBossId: fight.bossId,
			pickIndex: pickIndex++,
		});
	}

	const lastFight = bossFights[bossFights.length - 1];
	const result = lastFight.result === 'won' && lastFight.bossId === 'MetaProgression.BossID.Ordan' ? 'cleared' : 'died';
	const started = new Date(Date.UTC(2026, 6, 12, 9, 0, 0) + index * 3.6e6);

	return {
		schema: 1,
		runId: `run-${String(index).padStart(4, '0')}`,
		seed: 8000 + index * 37,
		startedAtUtc: started.toISOString().replace('.000', ''),
		durationSec: round(bossFights.reduce((s, f) => s + f.durationSec, 0) + 60 + rnd() * 90, 1),
		weapon: WEAPONS[index % 3 === 0 ? 1 : index % 5 === 0 ? 2 : 0],
		result,
		metaUpgrades: {
			Strength: Math.floor(skill * 4),
			Vitality: Math.floor(skill * 3),
			Luck: Math.floor(skill * 2),
		},
		currencyEarned: {
			'MetaProgression.Currency.AshSoul': Math.round(180 + skill * 420 + rnd() * 120),
			'MetaProgression.Currency.CathedralSigil': bossFights.filter((f) => f.result === 'won').length,
			'MetaProgression.Currency.RelicFragment': result === 'cleared' ? 1 : 0,
		},
		cardsPicked,
		bossFights,
	};
}

/* ── 출력 ─────────────────────────────────────────────────────── */

mkdirSync(RUNS, { recursive: true });

const timeline = buildTimeline();
writeFileSync(resolve(MOCK, 'sample-timeline.json'), JSON.stringify(timeline, null, '\t'));

const index = [];
for (let i = 1; i <= 20; i++) {
	const run = buildRun(i);
	const file = `run-${String(i).padStart(4, '0')}.json`;
	writeFileSync(resolve(RUNS, file), JSON.stringify(run, null, '\t'));
	index.push({ id: run.runId, file: `runs/${file}` });
}
// /api/runs 응답과 같은 모양의 목록 — 클라이언트가 같은 코드 경로로 읽는다
writeFileSync(resolve(RUNS, 'index.json'), JSON.stringify(index, null, '\t'));

const gaugeCount = timeline.events.filter((e) => e.type === 'gauge').length;
console.log(`sample-timeline.json — ${timeline.events.length} events (gauge ${gaugeCount}), ${timeline.events.at(-1).t.toFixed(1)}s`);
console.log(`mock/runs — ${index.length} runs`);
