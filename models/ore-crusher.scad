use <lib.scad>

hd = 72;
ld = 16;
vld = 8;

module frameHD($fn=hd) {
	translate([0,0,1.5]) difference() {
		fillet(0.01,$fn=8) union() {
			box([4.8,4.8,3]);
			translate([0,0,0.75]) cyl(2,2,3,$fn=hd);
			translate([2.23,2.23,0]) box([0.5,0.5,3]);
			translate([2.23,-2.23,0]) box([0.5,0.5,3]);
			translate([-2.23,2.23,0]) box([0.5,0.5,3]);
			translate([-2.23,-2.23,0]) box([0.5,0.5,3]);
		}
		box([6,3,1]);
		box([3,6,1]);
		translate([0,0,1.5]) cyl(1.8,1.8,3);
	}
}

module frameLD($fn=ld) {
	translate([0,0,1.5]) difference() {
		union() {
			box([4.8,4.8,3]);
			translate([0,0,0.75]) cyl(2,2,3);
		}
		box([6,3,1]);
		box([3,6,1]);
		translate([0,0,1.5]) cyl(1.8,1.8,3);
	}
}

module frameVLD($fn=vld) {
	translate([0,0,1.5]) difference() {
		union() {
			box([4.8,4.8,3]);
			translate([0,0,0.75]) cyl(2,2,3);
		}
	}
}

module rotor() {
	translate([0,0,3.5]) difference() {
		cyl(1.79,1.79,1);
		if ($fn > 8) {
			for (i = [0:1:12]) translate([0,0,1]) rotate([0,0,i*30]) translate([0,1,0]) rotate([-10,0,0]) box([0.3,2,1]);
		}
	}
}

module side() {
	translate([0,-2,1.5]) box([2.99,1,0.99]);
}

//frameHD();
//frameVLD();
rotor($fn=vld);
//side($fn=d);
