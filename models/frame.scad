use <lib.scad>

module frameHD() {
	difference() {
		rbox([0.6,0.6,0.6],0.01,$fn=8);
		box([0.4,0.4,0.7]);
		box([0.4,0.7,0.4]);
		box([0.7,0.4,0.4]);
	}
}

module frameLD() {
	difference() {
		box([0.6,0.6,0.6]);
		box([0.4,0.4,0.7]);
		box([0.4,0.7,0.4]);
		box([0.7,0.4,0.4]);
	}
}

frameLD();