use <lib.scad>

module body() {
	hull() {
		translate([0,0,0]) scale([3.15,1,1]) sphere(1.9);
		translate([2.6,0,0]) scale([1.75,1,1]) sphere(1.95);
	}
}

module tailfin() {
	hull() {
		translate([0,0,0]) sphere(0.02);
		translate([-1.6,0,1.6]) sphere(0.02);
		translate([-3,0,1.6]) sphere(0.02);
		translate([-3,0,0]) sphere(0.02);
	}
}

module gondola() {
	hull() {
		translate([1.2,0.6,0.6]) sphere(0.02);
		translate([0.6,0.6,-0.6]) sphere(0.02);
		translate([1.2,-0.6,0.6]) sphere(0.02);
		translate([0.6,-0.6,-0.6]) sphere(0.02);
		translate([-1.2,0.6,0.6]) sphere(0.02);
		translate([-0.6,0.6,-0.6]) sphere(0.02);
		translate([-1.2,-0.6,0.6]) sphere(0.02);
		translate([-0.6,-0.6,-0.6]) sphere(0.02);
	}
}

module engine() {
	intersection() {
		scale([1.5,1,1]) sphere(0.4);
		translate([0.8,0,0]) box([2,2,2]);
	}
}

module blade() {
	translate([0.25,0,0]) rotate([30,0,0]) scale([9,4,1]) cyl(0.03, 0.03, 0.01);
}

module propeller() {
	rotate([0,90,0]) union() {
		cyl(0.025, 0.025, 0.2);
		translate([0,0,-0.05]) blade();
		translate([0,0,-0.05]) rotate([0,0,90]) blade();
		translate([0,0,-0.05]) rotate([0,0,180]) blade();
		translate([0,0,-0.05]) rotate([0,0,270]) blade();
	}
}

module spar() {
	box([0.05, 0.05, 1]);
}

hd = 32;
ld = 16;
vld = 8;
raise = 5;

hdFin = 8;
ldFin = 4;
vldFin = 2;

//body($fn=vld);
//translate([2.5,0,-1.6]) gondola($fn=hd);
//
//module engineAssembly() {
//	engine($fn=hd);
//	translate([-0.3,0,0]) propeller($fn=hd);
//	rotate([-35,0,0]) translate([0,0,0.8]) spar($fn=hd);
//	rotate([35,0,0]) translate([0,0,0.8]) spar($fn=hd);
//	rotate([0,45,0]) translate([0,0,0.8]) spar($fn=hd);
//}
//
//rotate([40,0,0]) translate([-3,0,-2.2]) engineAssembly();
//rotate([-40,0,0]) translate([-3,0,-2.2]) engineAssembly();
//
//translate([-2.6,0,0]) rotate([0,0,0]) tailfin($fn=vldFin);
//translate([-2.6,0,0]) rotate([90,0,0]) tailfin($fn=vldFin);
//translate([-2.6,0,0]) rotate([180,0,0]) tailfin($fn=vldFin);
//translate([-2.6,0,0]) rotate([270,0,0]) tailfin($fn=vldFin);
//translate([5,0.7,0]) rotate([270,0,0]) tailfin($fn=hdFin);
//translate([5,-0.7,0]) rotate([90,0,0]) tailfin($fn=hdFin);

//body($fn=ld);
//gondola($fn=ld/2);
//tailfin($fn=vldFin);
//engine($fn=hd);
//propeller($fn=ld/2);
//spar($fn=hd);
