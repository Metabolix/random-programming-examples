var near_z = 0.2, near_w = 0.4, near_h;

var levelImages = {
	images: {},
	findImages: function() {
		var arr = document.getElementById("imgLevel").getElementsByTagName("img");
		for (var i = 0; i < arr.length; ++i) {
			var img = new Image();
			img.src = arr[i].src;
			img.alt = arr[i].alt;
			this.images[arr[i].title] = img;
		}
	},
	get: function(name) {
		return this.images[name];
	}
};

var playerImages = {
	images: [],
	findImages: function() {
		var table = document.getElementById("imgPlayer");
		for (var i = 0; i < table.rows.length; ++i) {
			this.images.push([]);
			for (var j = 0; j < table.rows[i].cells.length; ++j) {
				var tmp = table.rows[i].cells[i].getElementsByTagName("img");
				this.images[i].push(tmp.length ? tmp[0] : null);
			}
		}
	},
	get: function(orientation, stage) {
		orientation = Math.modPositive(Math.round(orientation / (2 * Math.PI) * this.images.length), this.images.length);
		stage = Math.floor(Math.modPositive(stage, 1) * this.images[orientation].length);
		var e;
		try {
			return this.images[orientation][stage];
		} catch (e) {
			return null;
		}
	}
};

var ballImages = {
	images: [],
	findImages: function() {
		var arr = document.getElementById("imgBall").getElementsByTagName("img");
		for (var i = 0; i < arr.length; ++i) {
			var img = new Image();
			img.src = arr[i].src;
			img.alt = arr[i].alt;
			this.images.push(img);
		}
	},
	get: function(time) {
		return this.images[Math.floor((time % 1) * this.images.length)];
	}
};

var explosionImages = {
	images: [],
	findImages: function() {
		var arr = document.getElementById("imgExplosion").getElementsByTagName("img");
		for (var i = 0; i < arr.length; ++i) {
			var img = new Image();
			img.src = arr[i].src;
			img.alt = arr[i].alt;
			this.images.push(img);
		}
	},
	get: function(time) {
		return this.images[Math.floor(((time / explosions.duration) % 1) * this.images.length)];
	}
};

function stepNN(x, y, dx, dy) {
	if (x % 1 == 0 && y % 1 == 0) return 1;
	if (x % 1 == 0) return (y % 1) / -dy;
	if (y % 1 == 0) return (x % 1) / -dx;
	return Math.min((x % 1) / -dx, (y % 1) / -dy);
}

function stepNP(x, y, dx, dy) {
	if (x % 1 == 0 && y % 1 == 0) return 1;
	if (x % 1 == 0) return (1 - (y % 1)) / dy;
	if (y % 1 == 0) return (x % 1) / -dx;
	return Math.min((x % 1) / -dx, (1 - (y % 1)) / dy);
}

function stepPP(x, y, dx, dy) {
	if (x % 1 == 0 && y % 1 == 0) return 1;
	if (x % 1 == 0) return (1 - (y % 1)) / dy;
	if (y % 1 == 0) return (1 - (x % 1)) / dx;
	return Math.min((1 - (x % 1)) / dx, (1 - (y % 1)) / dy);
}

function stepPN(x, y, dx, dy) {
	if (x % 1 == 0 && y % 1 == 0) return 1;
	if (x % 1 == 0) return (y % 1) / -dy;
	if (y % 1 == 0) return (1 - (x % 1)) / dx;
	return Math.min((1 - (x % 1)) / dx, (y % 1) / -dy);
}

function step1(x, y, dx, dy) {
	return 0.9;
}

function calcWorldCol(x, y, dir, col) {
	var dx = Math.cos(dir);
	var dy = Math.sin(dir);
	var dist = 0;
	var wall;

	var stepFunc = ((!dx || !dy) ? step1 : ((dx < 0) ? ((dy < 0) ? stepNN : stepNP) : ((dy < 0) ? stepPN : stepPP)));
	var step_count = 0;
	var avoid_rounding_problems = 0.001;
	while (!wall) {
		var step = stepFunc(x, y, dx, dy) + avoid_rounding_problems;
		dist += step;
		x += step * dx;
		y += step * dy;
		++step_count;
		wall = level.get(x, y);
	}

	var img_x;
	if (Math.min(x % 1, 1 - x % 1) > Math.min(y % 1, 1 - y % 1)) {
		if (dy > 0) {
			img_x = (x % 1);
		} else {
			img_x = (1 - (x % 1));
		}
	} else {
		if (dx > 0) {
			img_x = (1 - (y % 1));
		} else {
			img_x = (y % 1);
		}
	}
	return {
		"func": drawWorldCol,
		"dist": dist,
		"wall": wall,
		"img_x": img_x,
		"col": col
	};
}

function calcPos(x, y, dir0, pos) {
	var dir = Math.modPositive(Math.atan2(pos.y - y, pos.x - x) - dir0, 2 * Math.PI);
	if (dir >= Math.PI / 2 && 3 * Math.PI / 2 >= dir) {
		return null;
	}

	var dist = Math.hypot(pos.y - y, pos.x - x);
	if (dist < near_z / 100) {
		return null;
	}

	return {
		"dist": dist,
		"dir": dir
	};
}

function calcPlayer(x, y, dir0, p) {
	var tmp = calcPos(x, y, dir0, p.pos);
	if (tmp) {
		tmp.orientation = p.dir - dir0;
		tmp.player = p;
		tmp.func = drawPlayer;
	}
	return tmp;
}

function calcBall(x, y, dir0, ball) {
	var tmp = calcPos(x, y, dir0, ball.pos);
	if (tmp) {
		tmp.time = ball.time;
		tmp.func = drawBall;
	}
	return tmp;
}

function calcExplosion(x, y, dir0, explosion) {
	var tmp = calcPos(x, y, dir0, explosion.pos);
	if (tmp) {
		tmp.time = explosion.time;
		tmp.func = drawExplosion;
	}
	return tmp;
}

function drawWorldCol(context, options, info) {
	var canvas_w = context.canvas.width;
	var canvas_h = context.canvas.height;
	var near_hypot = Math.hypot(near_z, near_w * (info.col / (canvas_w - 1) - 0.5));
	var wall_h = near_hypot / info.dist / near_h;
	var y0 = (1 - wall_h) * canvas_h / 2;
	var h = wall_h * canvas_h;

	var image = levelImages.get(info.wall);
	if (options.texture && image) {
		context.drawImage(image, Math.floor(info.img_x * image.width), 0, 1, image.height, info.col, y0, 1, h);
	} else {
		context.fillStyle = (image ? image.alt : "#969");
		context.fillRect(info.col, y0, 1, h);
	}
	if (options.shadow) {
		context.save();
		context.fillStyle = "#000";
		context.globalAlpha = Math.max(1 - wall_h * 3, 0.1);
		context.fillRect(info.col, y0, 1, h);
		context.restore();
	}
}

function drawPosHelper(context, info) {
	var canvas_w = context.canvas.width;
	var canvas_h = context.canvas.height;
	var col = Math.round((0.5 - Math.tan(info.dir) * near_z / near_w) * (canvas_w - 1));
	var near_hypot = Math.hypot(near_z, near_w * (col / (canvas_w - 1) - 0.5));
	var draw_unit = near_hypot / info.dist * canvas_h / near_h;
	var y0 = (canvas_h - draw_unit) / 2;

	info.draw_unit = draw_unit;
	info.draw_x = col;
	info.draw_y = canvas_h / 2;
}

function drawPlayer(context, options, info) {
	var image = playerImages.get(info.orientation, info.player.animation);

	drawPosHelper(context, info);
	var x = info.draw_x;
	var y = info.draw_y + 0.1 * info.draw_unit;
	var h = 0.8 * info.draw_unit;
	var w = h * (image ? (image.width / image.height) : 0.5);
	var x0 = x - w / 2;
	var y0 = y - h / 2;

	if (image) {
		context.drawImage(image, x0, y0, w, h);
	} else {
		context.fillStyle = "green";
		context.fillRect(x0, y0, w, h);
	}
	var str = info.player.name + " (hp: " + info.player.hp + ", score: " + info.player.score + ")";
	context.textAlign = "center";
	context.fillStyle = "white";
	context.fillText(str, x - 1, y0 - 1);
	context.fillStyle = "black";
	context.fillText(str, x, y0);
}

function drawImage(context, options, info, rel_h, image, alt) {
	drawPosHelper(context, info);
	var h = info.draw_unit * rel_h;
	var w = h;
	var x0 = info.draw_x - w / 2;
	var y0 = info.draw_y - h / 2;

	if (image) {
		w *= image.width / image.height;
		context.drawImage(image, x0, y0, w, h);
	} else {
		context.fillStyle = alt;
		context.fillRect(x0, y0, w, h);
	}
}

function drawBall(context, options, info) {
	drawImage(context, options, info, 0.2, ballImages.get(info.time), "red");
}

function drawExplosion(context, options, info) {
	drawImage(context, options, info, 0.3, explosionImages.get(info.time), "orange");
}

function sortByDistDesc(a, b) {
	return b.dist - a.dist;
}

function drawWorld(context, options) {
	var canvas_w = context.canvas.width;
	var canvas_h = context.canvas.height;
	near_h = near_w / canvas_w * canvas_h;

	var objects = [];
	for (var col = 0; col < canvas_w; ++col) {
		var dir = player.dir + Math.atan2(near_w * (0.5 - col / (canvas_w - 1)), near_z);
		var tmp = calcWorldCol(player.pos.x, player.pos.y, dir, col);
		if (tmp) objects.push(tmp);
	}
	players.each(function(other) {
		if (other != player) {
			var tmp = calcPlayer(player.pos.x, player.pos.y, player.dir, other);
			if (tmp) objects.push(tmp);
		}
	});
	for (var id in balls.data) {
		var tmp = calcBall(player.pos.x, player.pos.y, player.dir, balls.data[id]);
		if (tmp) objects.push(tmp);
	}
	for (var i = 0; i < explosions.length; ++i) {
		var tmp = calcExplosion(player.pos.x, player.pos.y, player.dir, explosions[i]);
		if (tmp) objects.push(tmp);
	}
	objects.sort(sortByDistDesc);

	var context = canvas.getContext('2d');
	context.fillStyle = "blue";
	context.fillRect(0, 0, canvas_w, canvas_h / 2);
	context.fillStyle = "gray";
	context.fillRect(0, canvas_h / 2, canvas_w, canvas_h);

	for (var i = 0; i < objects.length; ++i) {
		objects[i].func(context, options, objects[i]);
	}
}

function drawMap(context, options) {
	var canvas_w = context.canvas.width;
	var canvas_h = context.canvas.height;

	context.fillStyle = "black";
	context.fillRect(0, 0, canvas_w, canvas_h);
	context.save();
	context.translate(canvas_w / 2, canvas_h / 2);
	context.scale(8, -8);

	context.save();
	context.rotate(-player.dir + Math.PI / 2);
	context.translate(-player.pos.x, -player.pos.y);
	for (var y = 0; y < level.h; ++y) {
		for (var x = 0; x < level.w; ++x) {
			var wall = level.get(x, y);
			if (!wall) {
				continue;
			}
			var image = levelImages.get(wall);
			if (options.texture && image) {
				context.drawImage(image, x, y, 1, 1);
			} else {
				context.fillStyle = (image ? image.alt : "#969");
				context.fillRect(x, y, 1, 1);
			}
		}
	}
	context.restore();

	context.fillStyle = "red";

	context.rotate(-Math.PI / 2);

	context.save();
	context.rotate(-Math.PI / 6);
	context.fillRect(-0.2, -0.2, 1, 0.4);
	context.restore();

	context.save();
	context.rotate(Math.PI / 6);
	context.fillRect(-0.2, -0.2, 1, 0.4);
	context.restore();

	context.restore();
}
