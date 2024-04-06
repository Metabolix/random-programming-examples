if (!Math.hypot) {
	Math.hypot = function(dx, dy) {
		return Math.sqrt(dx * dx + dy * dy);
	};
}

if (!Math.modPositive) {
	Math.modPositive = function(a, b) {
		return (a % b + b) % b;
	};
}

if (!Math.round2) {
	Math.round2 = function(a, step) {
		return Math.round(a / step) * step;
	};
}
