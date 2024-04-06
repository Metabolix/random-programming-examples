function Input() {
	this.up = this.down = this.left = this.right = false;
};

Input.prototype.KEY = {
	D: 68, RIGHT: 39,
	W: 87, UP: 38,
	A: 65, LEFT: 37,
	S: 83, DOWN: 40,
	CTRL: 17, SPACE: 32,
	"":""
};

Input.prototype.copyTo = function(other) {
	other.up = this.up;
	other.down = this.down;
	other.left = this.left;
	other.right = this.right;
};

Input.prototype.getInput = function(code, is_down) {
	switch (code) {
		case this.KEY.W: case this.KEY.UP: this.up = is_down; break;
		case this.KEY.S: case this.KEY.DOWN: this.down = is_down; break;
		case this.KEY.A: case this.KEY.LEFT: this.left = is_down; break;
		case this.KEY.D: case this.KEY.RIGHT: this.right = is_down; break;
		case this.KEY.CTRL: case this.KEY.SPACE: this.shoot = is_down; break;
	}
};

var input = new Input();
