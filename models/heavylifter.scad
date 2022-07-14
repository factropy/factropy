use <lib.scad>

small = [2.95, 6.95, 3];
//translate([0,0,1.5]) box(small);

module engine() {
	difference() {
		hull() {
			cyl(0.5,0.5,0.5);
			translate([0,0,0.5]) torus(0.1,0.25);
			translate([0,0,-0.5]) torus(0.1,0.25);
		}
		cyl(0.3,0.3,2.0);
	}
}

module fan() {
	translate([0,0,0.45]) union() {
		for (i = [0:10:360]) {
			rotate([0,0,i]) translate([0,0.15,0]) rotate([0,20,0]) box([0.01,0.3,0.1]);
		}
		hull() {
			translate([0,0,0.05]) sphere(0.1);
			translate([0,0,-0.7]) sphere(0.1);
		}
	}
}

module nozzle() {
	translate([0,0,-0.5]) cyl(0, 0.2, 0.5);
}

module shaft() {
	union() {
		fan();
		nozzle();
	}
}

module engineAssembly() {
	engine($fn=32);
	fan($fn=16);
	nozzle($fn=16);
}

module tailwing(l) {
	hull() {
		translate([0,0.3,0]) rotate([0,90,0]) cyl(0.08,0.08,l);
		translate([0,-0.3,0]) rotate([0,90,0]) cyl(0.08,0.08,l);
	}
}

module tailfin() {
	hull() {
		translate([0,3,0]) sphere(0.05);
		translate([0,0,0]) sphere(0.05);
		translate([0,3,1]) sphere(0.05);
		translate([0,1.5,1]) sphere(0.05);
	}
}

module mainwing(l) {
	union() {
		hull() {
			translate([1,0.75,0]) sphere(0.1);
			translate([1,-0.75,0]) sphere(0.1);
			translate([-1,0.75,0]) sphere(0.1);
			translate([-1,-0.75,0]) sphere(0.1);
		}

		hull() {
			translate([-2.6,0.25,-0.5]) sphere(0.1);
			translate([-2.6,-0.25,-0.5]) sphere(0.1);
			translate([-1,0.75,0]) sphere(0.1);
			translate([-1,-0.75,0]) sphere(0.1);
		}

		hull() {
			translate([2.6,0.25,-0.5]) sphere(0.1);
			translate([2.6,-0.25,-0.5]) sphere(0.1);
			translate([1,0.75,0]) sphere(0.1);
			translate([1,-0.75,0]) sphere(0.1);
		}
	}
}

module body() {
	intersection() {
		hull() {
			translate([0,-1.5,0]) scale([1,1.5,1]) torus(0.5,0.75);
			translate([0,-2,-0.25]) sphere(1.5);
			translate([0,5,0.3]) sphere(0.1,$fn=$fn/4);
			translate([0,2.5,-0.4]) sphere(0.1,$fn=$fn/4);
		}
		translate([0,0,0.5]) box([5,12,2]);
	}
}

module frame() {
	box([1,3.5,0.1]);
	translate([0,2,0]) box([3.2,1,0.1]);
	translate([0,-2,0]) box([3.2,1,0.1]);
}

module arm() {
	union() {
		hull() {
			translate([0,0,0]) box([0.1,1,0.1]);
			translate([0,0,-3.1]) box([0.1,0.5,0.1]);
		}

		hull() {
			translate([0,0,-3.1]) box([0.1,0.5,0.1]);
			translate([-0.2,0,-3.1]) box([0.1,0.5,0.1]);
		}
	}
}

up=3.6;

//translate([3,0,up+0.4]) engineAssembly();
//translate([-3,0,up+0.4]) engineAssembly();
//translate([1.5,4,up+0.3]) rotate([90,0,0]) engineAssembly();
//translate([-1.5,4,up+0.3]) rotate([90,0,0]) engineAssembly();
//
//translate([0,4,up+0.3]) tailwing(2.1,$fn=12);
//
//translate([0,1.75,up+0.3]) tailfin($fn=8);
//
//translate([0,0,up+0.9]) mainwing(5.5,$fn=16);
//
//translate([0,0,up]) body($fn=64);
//
//translate([0,0,up-0.55]) frame();
//
//translate([1.6,-2,up-0.55]) arm();
//translate([-1.6,-2,up-0.55]) rotate([0,0,180]) arm();
//translate([1.6,2,up-0.55]) arm();
//translate([-1.6,2,up-0.55]) rotate([0,0,180]) arm();

//engine($fn=8);
//shaft($fn=16);
//tailwing(2.1,$fn=4);
//tailfin($fn=4);
//mainwing(5.5,$fn=4);
//body($fn=4);
//frame();
//arm();
