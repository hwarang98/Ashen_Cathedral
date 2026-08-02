// src/lib/palette.ts 의 계열 색을 두 모드 모두 검증한다.
// 색이나 계열 순서를 바꿨다면 반드시 이 스크립트를 다시 돌려 통과를 확인할 것.
//   npm run validate:palette
// 종료 코드가 0이 아니면 팔레트를 다시 골라야 한다.

import { validate } from './validate_palette.js';

// palette.ts 의 GAUGE_SERIES 와 같은 순서 — 순서가 곧 CVD 안전장치다
const DARK = ['#e66767', '#3987e5', '#c98500', '#199e70', '#d95926'];
const LIGHT = ['#e34948', '#2a78d6', '#eda100', '#1baf7a', '#eb6834'];
const DARK_SURFACE = '#17171b';
const LIGHT_SURFACE = '#fbfbf9';
const LABELS = ['Health', 'GuardGauge', 'Posture', 'Stamina', 'BurnGauge'];

let failed = false;

for (const [mode, palette, surface] of [
	['dark', DARK, DARK_SURFACE],
	['light', LIGHT, LIGHT_SURFACE],
]) {
	// 선 차트이므로 인접쌍 pairlist 를 쓴다 (scatter/map 이면 pairs: 'all')
	const { report, ok } = validate(palette, { mode, surface, pairs: 'adjacent' });
	console.log(`\n[${mode}] surface ${surface} — ${LABELS.join(' → ')}`);
	for (const [check, status, detail] of report) {
		const tag = status === true || status === 'pass' ? 'PASS' : status === 'floor' || status === 'relief' ? 'WARN' : 'FAIL';
		console.log(`  [${tag}] ${check.padEnd(22)} ${detail}`);
	}
	if (!ok) failed = true;
}

console.log(
	failed
		? '\n→ FAILED — 색을 다시 고르거나 계열 순서를 바꿀 것'
		: '\n→ 두 모드 모두 통과'
);
process.exit(failed ? 1 : 0);
