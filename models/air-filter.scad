use <lib.scad>

module cutout() {
	difference() {
		cyl(0.29,0.29,0.5);
		cyl(0.20,0.20,0.6);
	}
	translate([0,0,0.31]) cyl(0.18,0.18,0.1);
}

module frame() {
	union() {
		difference() {
			cyl(0.28,0.28,0.6);
			cutout();
		}
		translate([0,0,0.27]) bolt();
	}
}

module mesh() {
	difference() {
		cyl(0.26,0.26,0.49);
		cyl(0.25,0.25,0.6);
		for (i = [-2:1:2])
			for (j = [0:30:150])
				translate([0,0,0.1*i]) rotate([0,0,j+(i*15)]) rotate([0,90,0]) cyl(0.06,0.06,1.0,$fn=6);
	}
}

module bolt() {
	cyl(0.06,0.06,0.05,$fn=6);
}

module filter() {
	cyl(0.24,0.24,0.5);
}

module frameLD($fn=8) {
	frame();
}

module frameHD($fn=24) {
	frame();
}

module meshHD($fn=24) {
	mesh();
}

module filterHD($fn=24) {
	filter();
}

module filterLD($fn=8) {
	filter();
}

module all() {
	translate([0,0,0.25])
	frameHD();
	translate([0,0,0.25])
	meshHD();
	translate([0,0,0.25])
	filterLD();
}

//all();

translate([0,0,0.3])
//frameHD();
frameLD();
//meshHD();
//filterHD();
//filterLD();
