use <lib.scad>

hd=36;
ld=12;

footing=2.5;

module baseHD($fn=hd) {
	translate([0,0,0.5]) rbox([5,5,1],0.01,$fn=8);
}

module baseLD($fn=ld) {
	translate([0,0,0.5]) box([5,5,1]);
}

module casingHD($fn=hd) {
	translate([0,0,footing]) union() {
		difference() {
			cyl(0.25, 0.25, 3.0);
			cyl(0.20, 0.20, 2.9);
			box([0.1, 0.6, 2.8]);
			rotate([0,0,90]) box([0.1, 0.6, 2.8]);
		}
		translate([0,0,1.5-0.025]) cyl(0.3,0.3,0.05);
		translate([0,0,-1.5+0.025]) cyl(0.3,0.3,0.05);
	}
}

module casingLD($fn=ld) {
	translate([0,0,footing]) union() {
		difference() {
			cyl(0.25, 0.25, 3.0);
			cyl(0.20, 0.20, 2.9);
			box([0.1, 0.6, 2.8]);
			rotate([0,0,90]) box([0.1, 0.6, 2.8]);
		}
		translate([0,0,1.5-0.025]) cyl(0.3,0.3,0.05);
		translate([0,0,-1.5+0.025]) cyl(0.3,0.3,0.05);
	}
}

module rotorHD($fn=hd) {
	translate([0,0,footing]) box([0.35, 0.1, 2.7]);
}

module rotorLD($fn=ld) {
	translate([0,0,footing]) box([0.35, 0.1, 2.7]);
}

module helixHD($fn=6) {
	translate([0,0,footing]) intersection() {
		for (i = [0:1:9])
			translate([0,0,i*0.3-1.5]) hull_chain() {
				rotate([0,0,0*18]) translate([0.26,0,0*0.015]) sphere(0.01);
				rotate([0,0,1*18]) translate([0.26,0,1*0.015]) sphere(0.01);
				rotate([0,0,2*18]) translate([0.26,0,2*0.015]) sphere(0.01);
				rotate([0,0,3*18]) translate([0.26,0,3*0.015]) sphere(0.01);
				rotate([0,0,4*18]) translate([0.26,0,4*0.015]) sphere(0.01);
				rotate([0,0,5*18]) translate([0.26,0,5*0.015]) sphere(0.01);
				rotate([0,0,6*18]) translate([0.26,0,6*0.015]) sphere(0.01);
				rotate([0,0,7*18]) translate([0.26,0,7*0.015]) sphere(0.01);
				rotate([0,0,8*18]) translate([0.26,0,8*0.015]) sphere(0.01);
				rotate([0,0,9*18]) translate([0.26,0,9*0.015]) sphere(0.01);
				rotate([0,0,10*18]) translate([0.26,0,10*0.015]) sphere(0.01);
				rotate([0,0,11*18]) translate([0.26,0,11*0.015]) sphere(0.01);
				rotate([0,0,12*18]) translate([0.26,0,12*0.015]) sphere(0.01);
				rotate([0,0,13*18]) translate([0.26,0,13*0.015]) sphere(0.01);
				rotate([0,0,14*18]) translate([0.26,0,14*0.015]) sphere(0.01);
				rotate([0,0,15*18]) translate([0.26,0,15*0.015]) sphere(0.01);
				rotate([0,0,16*18]) translate([0.26,0,16*0.015]) sphere(0.01);
				rotate([0,0,17*18]) translate([0.26,0,17*0.015]) sphere(0.01);
				rotate([0,0,18*18]) translate([0.26,0,18*0.015]) sphere(0.01);
				rotate([0,0,19*18]) translate([0.26,0,19*0.015]) sphere(0.01);
				rotate([0,0,20*18]) translate([0.26,0,20*0.015]) sphere(0.01);
			}
		box([3.0,3.0,2.99]);
	}
}

module helixLD($fn=8) {
}

module vesselHD() {
	casingHD();
	rotorHD();
	rotate([0,0,90]) rotorHD();
	helixHD();
}

module centrifugeHD() {
	baseHD();
	for (i = [-2:2], j = [-2:2]) translate([i,j,0]) vesselHD();
}

//centrifugeHD();

//baseHD();
//baseLD();
//casingHD();
//casingLD();
//rotorHD();
//rotorLD();
helixHD();
//helixLD();