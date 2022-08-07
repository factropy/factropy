use <lib.scad>

module triangle($fn=16) {
	radius=0.1;
	thick=0.2;
	bumpA=0.4-radius;
	bumpB=0.3-radius;
	rotate([90,0,0]) hull() {
		translate([-bumpA,-bumpB,0]) cyl(radius, radius, thick);
		translate([bumpA,-bumpB,0]) cyl(radius, radius, thick);
		translate([0,bumpA,0]) cyl(radius, radius, thick);
	}
}

module bolt() {
	scale([1,1,1.1]) rotate([0,5,0]) rotate([90,0,0]) difference() {
		scale([1,1.5,1]) cyl(0.15,0.15,0.25,$fn=4);
		translate([0.08,0.115,0]) rotate([0,0,30]) cyl(0.15,0.15,0.35,$fn=4);
		translate([-0.08,-0.115,0]) rotate([0,0,30]) cyl(0.15,0.15,0.35,$fn=4);
	}
}

//triangle();
bolt();
