use <lib.scad>

module nose() {
	scale([0.3,0.3,0.3]) intersection() {
		hull() {
			translate([0,0,-5]) scale([1,1,0.75]) BezCone(d=8.2,h=20,curve=5.1,steps=$fn);
			translate([0,0,9.5]) sphere(r=0.5);
		}
		translate([0,0,5]) box([10,10,10]);
	}
}

module body() {
	translate([0,0,10]) cyl(1.5,1.5,20);
}

module launchpad() {
	difference() {
		translate([0,0,1.5]) box([12,12,3]);
		translate([0,0,2.25]) cyl(2,2,2.0);
	}
}

module base() {
	translate([0,0,0.2]) difference() {
		box([3.5,3.5,0.5]);
		translate([2,2,0]) box([3,3,1]);
		translate([-2,2,0]) box([3,3,1]);
		translate([-2,-2,0]) box([3,3,1]);
		translate([2,-2,0]) box([3,3,1]);
	}
}

module leg() {
	translate([1.8,0,0.25]) rotate([0,0,0]) hull() {
		translate([0,0.8,0]) sphere(0.25);
		translate([0,-0.8,0]) sphere(0.25);
		translate([0,0,5]) sphere(0.25);
	}
}

module raptor() {
	intersection() {
		translate([0,0,-1]) BezCone(d=2,h=4,curve=1,steps=$fn);
		translate([0,0,2.5]) box([5,5,5]);
	}
}

module plume() {
	translate([0,0,3.25]) rotate([180,0,0]) BezCone(d=2,h=20,curve=1,steps=$fn);
}

hd = 72;
ld = 12;
raise = 6;

hdFin = 8;
ldFin = 4;

//translate([0,0,20+raise]) nose($fn=ld);
//translate([0,0,0+raise]) body($fn=ld);
//translate([0,0,0+raise]) base($fn=hd);
//translate([0,0,0+raise]) rotate([0,0,0]) leg($fn=ld);
//translate([0,0,0+raise]) rotate([0,0,90]) leg($fn=hd);
//translate([0,0,0+raise]) rotate([0,0,180]) leg($fn=hd);
//translate([0,0,0+raise]) rotate([0,0,270]) leg($fn=hd);
//
//rotate([0,0,0]) translate([0,0,4]) raptor($fn=hd);
//%rotate([0,0,0]) translate([0,0,1]) plume($fn=hd);
//
//launchpad($fn=hd);

hull() {
	translate([0,0,20]) nose($fn=hd);
	body($fn=hd);
}
//raptor($fn=ld);
//launchpad($fn=ld);
//translate([-1.8,0,-0.25]) leg($fn=hd);
//base($fn=hd);
