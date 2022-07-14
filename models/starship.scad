use <lib.scad>

module nose() {
	intersection() {
		hull() {
			translate([0,0,-5]) scale([1,1,0.75]) BezCone(d=8.2,h=20,curve=5.1,steps=$fn);
			translate([0,0,9.5]) sphere(r=0.5);
		}
		translate([0,0,5]) box([10,10,10]);
	}
}

module nosefin() {
	hull() {
		translate([0,0,0]) sphere(0.1);
		translate([-2,0,6]) sphere(0.1);
		translate([3,0,0]) sphere(0.1);
		translate([3,0,3]) sphere(0.1);
	}
}

module body() {
	translate([0,0,15]) cyl(5,5,30);
}

module tailfin() {
	hull() {
		translate([0,0,0]) sphere(0.1);
		translate([0,0,10]) sphere(0.1);
		translate([4,0,0]) sphere(0.1);
		translate([4,0,5]) sphere(0.1);
	}
}

module launchpad() {
	difference() {
		translate([0,0,1.5]) box([20,20,3]);
		translate([0,0,2.25]) cyl(4.2,4.2,2.0);
	}
}

module frame() {
	translate([0,0,3.25]) difference() {
		translate([0,0,-1]) cyl(4,4,4);
		cyl(3.8,3.8,4.2);
		for (i = [0:1:3])
			rotate([0,0,i*120]) box([15,3.5,1.5]);
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
raise = 5;

hdFin = 8;
ldFin = 4;

//translate([0,0,30+raise]) nose($fn=hd);
//translate([5,0,30+raise]) nosefin($fn=hd);
//translate([-5,0,30+raise]) rotate([0,0,180]) nosefin($fn=hd);
//translate([0,0,0+raise]) body($fn=hd);
//translate([5,0,0+raise]) tailfin($fn=hd);
//translate([-5,0,0+raise]) rotate([0,0,180]) tailfin($fn=hd);
//
//rotate([0,0,0]) translate([2,0,3.25]) raptor($fn=hd);
//rotate([0,0,120]) translate([2,0,3.25]) raptor($fn=hd);
//rotate([0,0,240]) translate([2,0,3.25]) raptor($fn=hd);
//
//%rotate([0,0,0]) translate([2,0,0]) plume($fn=hd);
//%rotate([0,0,120]) translate([2,0,0]) plume($fn=hd);
//%rotate([0,0,240]) translate([2,0,0]) plume($fn=hd);
//
//launchpad($fn=hd);
//translate([0,0,0.76]) frame($fn=hd);

//nose($fn=ld);
//nosefin($fn=ldFin);
//tailfin($fn=ldFin);
//body($fn=ld);
//raptor($fn=ld);
//launchpad($fn=hd);
//frame($fn=ld);
//plume($fn=ld);