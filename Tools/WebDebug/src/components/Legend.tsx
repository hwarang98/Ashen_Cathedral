// 계열 범례 — 2계열 이상이면 항상 표시한다. 색만으로 구분되게 두지 않는다.
// 토글해도 계열 색은 절대 바뀌지 않는다(색은 순위가 아니라 대상을 따라간다).

interface LegendItem {
	id: string;
	label: string;
	color: string;
	/** 선 계열이면 dash 패턴을 그대로 보여준다 (색 외 2차 인코딩) */
	dash?: number[];
	shape?: 'line' | 'dot';
}

interface Props {
	items: LegendItem[];
	hidden?: Set<string>;
	onToggle?: (id: string) => void;
}

export default function Legend({ items, hidden, onToggle }: Props) {
	return (
		<div className="legend" role="group" aria-label="계열 범례">
			{items.map((item) => {
				const on = !hidden?.has(item.id);
				const swatch =
					item.shape === 'dot' ? (
						<span className="legend__dot" style={{ background: item.color }} />
					) : (
						<span
							className="legend__swatch"
							style={{
								borderTopColor: item.color,
								borderTopStyle: item.dash && item.dash.length ? 'dashed' : 'solid',
							}}
						/>
					);
				return onToggle ? (
					<button
						key={item.id}
						type="button"
						className="legend__item"
						aria-pressed={on}
						onClick={() => onToggle(item.id)}
						title={on ? '숨기기' : '표시'}
					>
						{swatch}
						{item.label}
					</button>
				) : (
					<span key={item.id} className="legend__item">
						{swatch}
						{item.label}
					</span>
				);
			})}
		</div>
	);
}
