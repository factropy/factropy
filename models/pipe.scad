use <lib.scad>

hd=32;
ld=16;
vld=6;

module straight() {
	rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.4, 0.4, 1.0);
}

module window() {
	difference() {
		rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.4, 0.4, 1.0);
		rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.3, 0.3, 0.9);
		translate([0,0,0.25]) rbox([0.6,0.2,0.5],0.05,$fn=$fn/4);
	}
}

module windowFluid() {
	rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.3, 0.3, 0.9);
}

module cross() {
	union() {
		rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.4, 0.4, 1.0);
		rotate([0,180/$fn,0]) rotate([90,0,0]) cyl(0.4, 0.4, 1.0);
	}
}

module flange() {
	if ($fn >= ld) difference() {
		cyl(0.475, 0.475, 0.1);
		if ($fn > ld) {
			for (i = [0:45:360])
				#rotate([0,0,i]) translate([0.425,0,0]) cyl(0.025, 0.025, 0.11, $fn=$fn/4);
		}
	}
}

module ground() {
	union() {
		translate([-0.45,0,-0.45]) rotate([90,0,0]) intersection() {
			rotate_extrude(convexity=10) translate([0.45,0,0]) circle(r=0.4);
			translate([0.5,0.5,0]) box([1,1,1]);
		}
		translate([-0.45,0,0]) rotate([0,90,0]) flange();
		translate([0,0,-0.45]) flange();
	}
}

module tee() {
	union() {
		translate([0.25,0,0]) rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.4, 0.4, 0.5);
		translate([0,0.25,0]) rotate([0,180/$fn,0]) rotate([90,0,0]) cyl(0.4, 0.4, 0.5);
		translate([0,-0.25,0]) rotate([0,180/$fn,0]) rotate([90,0,0]) cyl(0.4, 0.4, 0.5);
	}
}

module item() {
	rotate([0,90,0]) difference() {
		union() {
			rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.375, 0.375, 0.9);
			translate([0.4,0,0]) rotate([0,90,0]) flange();
			translate([-0.4,0,0]) rotate([0,90,0]) flange();
		}
		rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.3, 0.3, 1.1);
	}
}

module valve() {
	union() {
		straight();
		translate([0,0,0.45]) union() {
			torus(0.05,0.3,$fn=$fn/2);
			rotate([0,90,0]) cyl(0.04,0.04,0.7,$fn=$fn/4);
			rotate([0,0,90]) rotate([0,90,0]) cyl(0.04,0.04,0.7,$fn=$fn/4);
		}
		translate([0,0,0.25]) cyl(0.1,0.1,0.5);
	}
}

module chevron() {
	rotate([90,0,0]) cyl(0.3,0.3,0.85, $fn=3);
}

module elbow() {
	rotate([0,-90,0]) rotate([0,0,-90]) ground();
}

//straight($fn=hd);
//straight($fn=ld);
//straight($fn=vld);
//cross($fn=hd);
//cross($fn=ld);
//cross($fn=vld);
//elbow($fn=hd);
//elbow($fn=ld);
//elbow($fn=vld);
//tee($fn=hd);
//tee($fn=ld);
//tee($fn=vld);
//ground($fn=hd);
//ground($fn=ld);
//ground($fn=vld);
//item($fn=hd);
//item($fn=ld);
//valve($fn=hd);
//valve($fn=ld);
//chevron($fn=ld);
//window($fn=hd*2);
window($fn=ld*2);
//windowFluid($fn=6);

