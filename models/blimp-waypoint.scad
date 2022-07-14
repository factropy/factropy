use <lib.scad>

hd = 72;
ld = 12;

module waypointHD() {
	rotate([0,0,22.5]) cyl(0.25, 0.25, 0.03, $fn=8);
}

waypointHD($fn=hd);
