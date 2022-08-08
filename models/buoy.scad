use <lib.scad>

hd = 16;
ld = 8;
vld = 6;

module frameHD($fn=hd) {
	thick=0.1;
	intersection() {
		union() {
			for (x = [-1:2:1]) for (y = [-1:2:1]) hull() {
				translate([0,0,3]) box([thick,thick,thick]);
				translate([x*-0.6,y*-0.6,0]) box([thick,thick,thick]);
			}
			translate([0,0,1.8]) difference() {
				box([0.5,0.5,0.1]);
				box([0.4,0.4,0.2]);
			}
			translate([0,0,0.8]) difference() {
				box([0.9,0.9,0.1]);
				box([0.8,0.8,0.2]);
			}
		}
		translate([0,0,1.5]) box([2,2,3]);
	}
}

module frameLD() {
	frameHD();
}

module ringHD($fn=hd) {
	torus(0.2, 0.6, $fn=$fn*4);
}

module ringLD($fn=hd) {
	torus(0.2, 0.6, $fn=$fn*2);
}

module frameVLD() {
	hull() {
		frameLD();
	}
}

//frameHD();
//frameLD();
frameVLD();
//ringHD();
//ringLD();
