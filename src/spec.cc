#include "common.h"
#include "sim.h"
#include "spec.h"

void Spec::reset() {
	for (auto [_,spec]: all) delete spec;
	all.clear();
}

Spec::Spec(std::string name) {
	ensuref(name.length() < 100, "names must be less than 100 characters");
	ensuref(all.count(name) == 0, "duplicate spec name %s", name.c_str());
	this->name = name;
	all[name] = this;

	highLOD = 0; // Config high LOD multiple

	coloredAuto = false;
	coloredCustom = false;
	color = 0xffffffff;
	tipStorage = false;

	licensed = false;
	align = true;
	store = false;
	shunt = false;
	junk = false;
	named = false;
	block = false;
	pile = false;
	underground = false;
	deconstructable = true;
	raise = false;
	enable = false;
	slab = false;

	rotateGhost = false;
	rotateExtant = false;
	rotations = {Point::South,Point::West,Point::North,Point::East};

	iconD = 0.0f;
	iconV = 0.0f;
	iconH = 0.0f;

	direct = false;
	directRevert = false;

	forceDelete = false;

	health = 0;

	explodes = false; // exploding entity, say missile
	explosive = false; // specifically a hill explosive
	explosion = false; // result of an exploding entity
	explosionSpec = nullptr;
	explosionDamage = 0;
	explosionRadius = 0;
	explosionRate = 0;

	capacity = 0;
	storeSetLower = false;
	storeSetUpper = false;
	storeMagic = false;
	storeUpgradePreserve = false;
	// store drones
	logistic = false;
	construction = false;
	overflow = false;
	// store arms
	priority = 0;

	showItem = false;
	showItemPos = Point::Zero;

	place = Land;
	placeOnHill = false;
	placeOnWaterSurface = false;
	placeOnHillPlatform = 0.0f;

	collision = {0,0,0,0,0,0};
	selection = {0,0,0,0,0,0};

	costGreedy = 1.0;
	clearance = 1.0;

	depot = false;
	depotAssist = false;
	depotFixed = false;
	depotRange = 0.0f;
	depotDrones = 0;
	depotDroneSpec = nullptr;
	depotDispatchEnergy = 0;

	consumeFuel = false;
	burnerState = false;
	consumeElectricity = false;
	consumeThermalFluid = false;
	consumeCharge = false;
	consumeChargeEffect = false;
	consumeChargeBuffer = 0;
	consumeChargeRate = 0;
	consumeMagic = false;
	energyConsume = 0;
	energyDrain = 0;
	generateElectricity = false;
	bufferElectricity = false;
	bufferElectricityState = false;
	bufferDischargeRate = 0;
	bufferChargeRate = 0;
	bufferCapacity = 0;
	energyGenerate = 0;
	generatorState = false;

	powerpole = false;
	powerpoleRange = 0;
	powerpoleCoverage = 0;
	powerpolePoint = Point::Zero;

	vehicle = false;
	vehicleStop = false;
	//vehicleEnergy = 0;
	vehicleWaitActivity = false;

	crafter = false;
	crafterMiner = false;
	crafterSmelter = false;
	crafterProgress = false;
	crafterOnOff = false;
	crafterState = false;
	crafterEnergyConsume = 0;
	crafterManageStore = true;
	crafterShowTab = true;
	crafterTransmitResources = false;
	crafterRate = 0.0f;
	crafterOutput = false;
	crafterOutputPos = Point::Zero;
	crafterRecipe = nullptr;

	effector = false;
	effectorElectricity = 0.0f;
	effectorFuel = 0.0f;
	effectorCharge = 0.0f;
	effectorEnergyDrain = 0;

	venter = false;
	venterRate = 0;

	launcher = false;
	launcherInitialState = 0;

	arm = false;
	armInput = Point::Zero;
	armOutput = Point::Zero;
	armSpeed = 0;

	drone = false;
	droneSpeed = 0.0f;
	dronePoint = Point::Zero;
	dronePointRadius = 0;
	droneCargoSize = 0;

	materialsMultiplyHill = 0;

	pipe = false;
	pipeHints = false;
	pipeValve = false;
	pipeCapacity = 0;
	pipeUnderground = false;
	pipeUnderwater = false;
	pipeUndergroundRange = 0.0f;
	pipeUnderwaterRange = 0.0f;

	turret = false;
	turretLaser = false;
	turretRange = 0;
	turretPivotSpeed = 0;
	turretCooldown = 0;
	turretDamage = 0;
	turretPivotPoint = Point::Zero;
	turretPivotFirePoint = Point::Zero;
	turretTrail = 0;
	turretStateAlternate = false;
	turretStateRevert = false;

	missile = false;
	missileSpeed = 0;
	missileBallistic = false;

	enemy = false;
	enemyTarget = false;

	biter = false;
	biterRange = 0.0f;
	biterSpeed = 0.0f;
	biterDamage = 0;

	conveyor = false;
	conveyorInput = Point::Zero;
	conveyorOutput = Point::Zero;
	conveyorCentroid = Point::Zero;
	conveyorEnergyDrain = 0;
	conveyorSlotsLeft = 0;
	conveyorSlotsRight = 0;
	unveyor = false;
	unveyorRange = 0.0f;
	unveyorEntry = false;
	unveyorPartner = nullptr;

	loader = false;
	loaderUnload = false;
	loaderPoint = Point::Zero;

	balancer = false;
	balancerLeft = Point::East;
	balancerRight = Point::West;

	computer = false;
	computerDataStackSize = 0;
	computerReturnStackSize = 0;
	computerRAMSize = 0;
	computerROMSize = 0;
	computerCyclesPerTick = 0;

	router = false;

	networker = false;
	networkHub = false;
	networkRange = 0.0f;
	networkWifi = Point::Zero;
	networkInterfaces = 0;

	cart = false;
	cartStop = false;
	cartWaypoint = false;
	cartSpeed = 0;
	cartWait = 0;
	cartItem = Point::Zero;
	cartRoute = nullptr;

	snapAlign = false;

	cycle = nullptr;
	cycleReverseDirection = false;

	follow = nullptr;
	followConveyor = nullptr;
	followReverseDirection = false;

	upward = nullptr;
	downward = nullptr;

	windTurbine = false;
	starship = false;

	zeppelin = false;
	zeppelinAltitude = 0.0f;
	zeppelinSpeed = 0.0f;

	flightPad = false;
	flightPadDepot = false;
	flightPadSend = false;
	flightPadRecv = false;
	flightPadHome = Point::Zero;
	flightPath = false;
	flightPathSpeed = 0.0f;
	flightPathClearance = 0.0f;
	flightLogistic = false;
	flightLogisticRate = 0;

	teleporter = false;
	teleporterEnergyCycle = 0;
	teleporterSend = false;
	teleporterRecv = false;

	pipette = nullptr;

	build = true;
	select = true;
	plan = true;
	clone = true;

	statsGroup = this;
	energyConsumption.clear();

	upgrade = nullptr;

	ruled = false;
	status = false;
	beacon = Point::Zero;
	icon = Point::Zero;

	tube = false;
	tubeOrigin = 0;
	tubeSpan = 0; // mm
	tubeSpeed = 0; // mm
	tubeGlass = nullptr;
	tubeRing = nullptr;
	tubeChevron = nullptr;

	monorail = false;
	monorailStop = false;
	monorailArrive = Point::Zero;
	monorailDepart = Point::Zero;
	monorailSpan = 0; // mm
	monorailAngle = 0;
	monorailStopUnload = Point::Zero;
	monorailStopEmpty = Point::Zero;
	monorailStopFill = Point::Zero;
	monorailStopLoad = Point::Zero;
	monorailContainer = false;

	monocar = false;
	monocarContainer = Point::Zero;
	monocarBogieA = 0;
	monocarBogieB = 0;
	monocarRoute = nullptr;
	monocarSpeed = 0.0;

	source = false;
	sourceItem = nullptr;
	sourceItemRate = 0;
	sourceFluid = nullptr;
	sourceFluidRate = 0;

	ship = false,
	shipRecipe = nullptr,

	shipyard = false;
	shipyardBuild = Point::Zero;
	shipyardLaunch = Point::Zero;

	once = false;
	done = false;

	field = false;
}

Spec::~Spec() {
}

Spec* Spec::byName(std::string name) {
	ensuref(all.count(name) == 1, "unknown spec name %s", name.c_str());
	return all[name];
}

Point Spec::aligned(Point p, Point dir) const {
	//p += collision.centroid();
	if (align) {
		float ww = collision.w;
		//float hh = h;
		float dd = collision.d;

		if (dir == Point::West || dir == Point::East) {
			ww = collision.d;
			dd = collision.w;
		}

		p.x = std::floor(p.x);
		if ((int)ceil(ww)%2 != 0) {
			p.x += 0.5;
		}

		if (alignStrict(p, dir)) {
			p.y = collision.h*0.5;
			if (underground) p.y = -collision.h*0.5;
			if (placeOnWaterSurface) p.y = -3.0f + collision.h*0.5;
		}

		p.z = std::floor(p.z);
		if ((int)ceil(dd)%2 != 0) {
			p.z += 0.5;
		}
	}
	return p;
}

bool Spec::alignStrict(Point pos, Point dir) const {
	float yMin = collision.h*0.5 - 0.01;
	float yMax = collision.h*0.5 + 0.01;
	bool grounded = pos.y > yMin && pos.y < yMax;
	bool undergrounded = pos.y < -yMin && pos.y > -yMax;
	return align && (grounded || undergrounded || placeOnWaterSurface)
		&& !drone && !zeppelin && !flightPath && !cart && !missile && !junk && !monocar && !vehicle;
}

// AABB
Box Spec::box(Point pos, Point dir, Box vol) const {
	pos += vol.centroid();

	float ww = vol.w;
	float dd = vol.d;

	if (dir != Point::North && dir != Point::South && dir != Point::East && dir != Point::West) {
		dir = dir.roundCardinal();
	}

	if (dir == Point::West || dir == Point::East) {
		ww = vol.d;
		dd = vol.w;
	}

	return {pos.x, pos.y, pos.z, ww, vol.h, dd};
}

// AABB
Box Spec::southBox(Point pos) const {
	return southBox(pos, collision);
}

Box Spec::southBox(Point pos, Box type) const {
	pos += type.centroid();
	return {pos.x, pos.y, pos.z, type.w, type.h, type.d};
}

Box Spec::placeOnHillBox(Point pos) const {
	pos += collision.centroid();

	float ww = placeOnHillPlatform;
	float dd = placeOnHillPlatform;

	return {pos.x, pos.y, pos.z, ww, collision.h, dd};
}

std::vector<Point> Spec::relativePoints(const std::vector<Point> points, const Mat4 rotation, const Point position) {
	std::vector<Point> rpoints;
	for (Point point: points) {
		rpoints.push_back(point.transform(rotation) + position);
	}
	return rpoints;
}

std::vector<Stack> Spec::constructionMaterials(float height) {
	std::vector<Stack> m;
	for (Stack stack: materials) {
		uint size = stack.size;
		if (materialsMultiplyHill > 0 && height > 0) {
			auto mul = std::ceil(materialsMultiplyHill * height);
			size += (float)size * mul;
		}
		m.push_back({stack.iid,size});
	}
	return m;
}

bool Spec::operable() const {
	return named
		|| vehicle
		|| cart
		|| cartStop
		|| cartWaypoint
		|| crafter
		|| launcher
		|| arm
		|| loader
		|| storeSetLower
		|| storeSetUpper
		|| generateElectricity
		|| pipeValve
		|| networker
		|| teleporter
		|| computer
		|| router
		|| depot
		|| balancer
		|| (tube && conveyor)
		|| monorail
		|| monocar
		|| powerpole
		|| pipeCapacity > Liquid::l(10000)
	;
}

bool Spec::directable() const {
	return zeppelin
		|| vehicle
		|| cart
		|| flightPath
	;
}

Rail Spec::railTo(Point posA, Point dirA, Spec* specB, Point posB, Point dirB) {
	Spec* specA = this;
	auto origin = posA + specA->monorailDepart.transform(dirA.rotation());
	auto target = posB + specB->monorailArrive.transform(dirB.rotation());
	return Rail(origin, dirA, target, dirB);
}

bool Spec::railOk(Rail& rail) {
	auto posA = rail.origin();
	auto posB = rail.target();
	auto distGround = posA.floor(0).distance(posB.floor(0));
	return distGround > 0.5 && rail.valid(((real)monorailSpan / 1000.0), monorailAngle);
}

Health Spec::damage(uint ammoId) const {
	if (!turretLaser && !ammoId) {
		ammoId = Item::bestAmmo();
		if (!ammoId) return 0;
	}
	return std::floor(
		turretLaser
			? (turretDamage)
			: ((float)(ammoId ? Item::get(ammoId)->ammoDamage: 0) * turretDamage)
	);
}

Point Spec::iconPoint(Point pos, Point dir) {
	Point i = icon == Point::Zero ? Point::Up*(collision.h/2+0.5f) : icon;
	return i.transform(dir.rotation()) + pos;
}

