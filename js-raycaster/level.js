var level = {
	data: [],
	nonempty: "A",
	w: 0, h: 0,
	starts: [],
	parse: function(str) {
		var lines = str.replace(/^\s+|\s+$/g, '').split(/[\r\n]+/);
		this.nonempty = str.match(/[^ X\n]/)[0];
		this.data = [];
		this.starts = [];
		this.h = lines.length;
		this.w = 0;
		for (var i = 0; i < lines.length; ++i) {
			this.data.push([]);
			for (var j = 0; j < lines[i].length; ++j) {
				var c = lines[i].substr(j, 1);
				if (c == 'X') {
					this.starts.push({
						"x": j + 0.5,
						"y": lines.length - i - 1 + 0.5
					});
					c = " ";
				}
				this.data[i].push(c);
			}
			this.w = Math.max(this.w, this.data[i].length);
		}
		if (this.starts.length == 0) {
			this.starts.push({"x": this.w / 2, "y": this.h / 2});
		}
		this.data.reverse();
	},
	get: function(x, y) {
		if (0 <= y && y < this.data.length) {
			y -= y % 1;
			if (0 <= x && x < this.data[y].length) {
				x -= x % 1;
				if (this.data[y][x] == " ") {
					return null;
				}
				return this.data[y][x];
			}
		}
		return this.nonempty;
	}
};
