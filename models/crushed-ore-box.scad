use <lib.scad>

module frame() {
	difference() {
		box([0.7,0.7,0.5]);
		translate([0,0,0.25]) box([0.61,0.61,0.5]);
	}
}

hd = 72;
ld = 12;

d = hd;

frame($fn=d);
