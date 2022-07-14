use <lib.scad>

hd = 64;
ld = 16;
vld = 8;

module flangeHD() {
	cyl(0.25,0.25,0.1);
}

module epipe() {
	x=0.65;
	z=2.6;
	points = [
		[x,0,-1],
		[x,0,(z-0.2)],
		[(x-0.07),0,(z-0.07)],
		[(x-0.2),0,z],
		[-(x-0.2),0,z],
		[-(x-0.07),0,(z-0.07)],
		[-x,0,(z-0.2)],
		[-x,0,-1],
	];
	rotate([0,0,90]) difference() {
		union() {
			pipe(points, 0.2);
			translate([-x,0,(z-0.3)]) flangeHD();
			translate([-(x-0.2),0,z]) rotate([0,90,0]) flangeHD();
			translate([x,0,(z-0.3)]) flangeHD();
			translate([(x-0.2),0,z]) rotate([0,90,0]) flangeHD();
		}
		translate([-0.5,0,-0.9]) box([1,1,1]);
		translate([0.5,0,0]) box([1,1,4]);
	}
}

module pipeHD($fn=hd/2) {
	translate([-0.45,-0.45,0]) rotate([0,0,-45]) epipe();
}

module pipeLD($fn=ld/2) {
	translate([-0.45,-0.45,0]) rotate([0,0,-45]) epipe();
}

module storageTankHD($fn=hd) {
	hull() {
		translate([0,0,2]) torus(0.25,0.45);
		translate([0,0,-0.25]) torus(0.25,0.45);
	}
}

module storageTankLD($fn=ld) {
	hull() {
		translate([0,0,2]) torus(0.25,0.45);
		translate([0,0,-0.25]) torus(0.25,0.45);
	}
}

module storageTankVLD($fn=vld) {
	hull() {
		translate([0,0,2]) torus(0.25,0.45);
		translate([0,0,-0.25]) torus(0.25,0.45);
	}
}

module mixingTankLD($fn=hd) {
	translate([0,0,0.25]) difference() {
		box([1.8,4,1.5]);
		translate([0,0,0.1]) box([1.6,3.8,1.5]);
	}
}

module mixingTankHD($fn=hd) {
	minkowskiOutsideRound(0.01,$fn=8)
	mixingTankLD();
}

module mixingTankVLD($fn=hd) {
	hull()
	mixingTankLD();
}

module mixingTankFluidHD($fn=hd) {
	#translate([0,0,0.25]) box([1.6,3.8,1.3]);
}

module agitatorHD($fn=hd) {
	union() {
		cyl(0.05,0.05,2,$fn=8);
		translate([0,0,0.75]) box([1.4,0.05,0.2]);
		translate([0,0,0]) rotate([0,0,90]) box([1.4,0.05,0.2]);
	}
}

module baseLD($fn=ld) {
	 difference() {
		hull() {
			translate([0,0,-1]) box([5.95, 5.95, 1]);
			translate([0,0,0]) box([5, 5, 0.1]);
		}
		translate([0,0,0.1]) box([4.75, 4.75, 1]);
	}
}

module baseHD($fn=hd) {
	minkowskiOutsideRound(0.01,$fn=8)
	baseLD();
}

module baseVLD($fn=ld) {
	union() {
		hull() baseLD();
		hull() for (i = [-2:1:2])
			translate([0,-i,0]) frameHD();
	}
}

module entity() {
	baseLD();
	translate([-1,-1,0]) storageTankHD();
	translate([-1,-1,0]) pipeHD();
	translate([-1,1,0]) storageTankHD();
	translate([-1,1,0]) pipeHD();
	translate([1.15,0,0]) mixingTankHD();
	translate([1.15,0,0]) mixingTankFluidHD();
	translate([1.15,1,0]) agitatorHD();
	translate([1.15,-1,0]) agitatorHD();
}

//translate([0,0,1.5]) entity();

//baseHD();
//baseLD();
//baseVLD();

//storageTankHD();
//storageTankLD();
//storageTankVLD();

//pipeHD();
//pipeLD();

//mixingTankHD();
//mixingTankLD();
//mixingTankVLD();

//mixingTankFluidHD();

agitatorHD();

//%box([5.95,5.95,3]);

