use <lib.scad>

module body() {
	hull() {
		scale([5,1,1]) sphere(5);
		translate([12.5,0,0]) scale([2.5,1,1]) sphere(5);
	}
}

module tailfin() {
	hull() {
		translate([0,0,0]) sphere(0.1);
		translate([-2,0,5]) sphere(0.1);
		translate([-8,0,5]) sphere(0.1);
		translate([-8,0,0]) sphere(0.1);
	}
}

module gondola() {
	hull() {
		translate([4,1.5,0]) sphere(0.1);
		translate([4,-1.5,0]) sphere(0.1);
		translate([2.5,1,-2]) sphere(0.1);
		translate([2.5,-1,-2]) sphere(0.1);
		translate([-4,1.5,0]) sphere(0.1);
		translate([-4,-1.5,0]) sphere(0.1);
		translate([-3,1,-2]) sphere(0.1);
		translate([-3,-1,-2]) sphere(0.1);
	}
}

module engine() {
	intersection() {
		scale([1.5,1,1]) sphere(0.5);
		translate([0.25,0,0]) box([1,1,1]);
	}
}

module blade() {
	translate([0.4,0,0]) rotate([30,0,0]) scale([15,2,1]) cyl(0.03, 0.03, 0.01);
}

module propeller() {
	rotate([0,90,0]) union() {
		cyl(0.05, 0.05, 0.5);
		translate([0,0,-0.2]) blade();
		translate([0,0,-0.2]) rotate([0,0,90]) blade();
		translate([0,0,-0.2]) rotate([0,0,180]) blade();
		translate([0,0,-0.2]) rotate([0,0,270]) blade();
	}
}

module spar() {
	box([0.05, 0.05, 5]);
}

hd = 72;
ld = 12;
raise = 5;

hdFin = 8;
ldFin = 4;

body($fn=hd);
translate([12,0,-4]) gondola($fn=hd);

module engineAssembly() {
	engine($fn=hd);
	translate([-0.25,0,0]) propeller($fn=hd);
	rotate([-35,0,0]) translate([0,0,2.5]) spar($fn=hd);
	rotate([35,0,0]) translate([0,0,2.5]) spar($fn=hd);
	rotate([0,45,0]) translate([0,0,2.5]) spar($fn=hd);
}

rotate([50,0,0]) translate([10,0,-6]) engineAssembly();
rotate([-50,0,0]) translate([10,0,-6]) engineAssembly();

rotate([40,0,0]) translate([0,0,-6]) engineAssembly();
rotate([-40,0,0]) translate([0,0,-6]) engineAssembly();

rotate([30,0,0]) translate([-10,0,-5.5]) engineAssembly();
rotate([-30,0,0]) translate([-10,0,-5.5]) engineAssembly();

translate([-15,0,0]) rotate([0,0,0]) tailfin($fn=hdFin);
translate([-15,0,0]) rotate([90,0,0]) tailfin($fn=hdFin);
translate([-15,0,0]) rotate([180,0,0]) tailfin($fn=hdFin);
translate([-15,0,0]) rotate([270,0,0]) tailfin($fn=hdFin);

//body($fn=ld);
//gondola($fn=ld);
//tailfin($fn=hdFin);
//engine($fn=ld);
//propeller($fn=ld);
//spar($fn=hd);
