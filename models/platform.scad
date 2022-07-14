use <lib.scad>

bounds = [10,10,1];
body = [9.99, 9.99, 0.99];

difference() {
	rbox(body,0.01,$fn=12);
}
