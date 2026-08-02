import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// mock/ 를 그대로 정적 루트로 쓴다 — 디스크 레이아웃은 mock/sample-timeline.json,
// mock/runs/*.json 이고 URL 은 /sample-timeline.json, /runs/*.json 이 된다.
// 빌드 산출물은 게임이 서빙하는 Saved/WebDebug 로 바로 떨어진다.
export default defineConfig({
	plugins: [react()],
	publicDir: 'mock',
	base: '/',
	build: {
		outDir: '../../Saved/WebDebug',
		emptyOutDir: true,
		target: 'es2020',
	},
	server: {
		port: 5173,
		strictPort: false,
	},
});
