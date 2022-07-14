#include "common.h"
#include "router.h"
#include "entity.h"
#include <cstdio>

// Router components connect wifi networks and can generate alerts based
// on packet rules.

void Router::reset() {
	all.clear();
}

void Router::tick() {
	for (auto& router: all) router.update();
}

Router& Router::create(uint id) {
	ensure(!all.has(id));
	Router& router = all[id];
	router.id = id;
	router.en = &Entity::get(id);
	router.networker = &Networker::get(id);
	router.alert.notice = 0;
	router.alert.warning = 0;
	router.alert.critical = 0;
	return router;
}

Router& Router::get(uint id) {
	return all.refer(id);
}

void Router::destroy() {
	all.erase(id);
}

RouterSettings::RouterSettings(Router& router) {
	rules = router.rules;
}

void Router::setup(RouterSettings* settings) {
	rules = settings->rules;
}

RouterSettings* Router::settings() {
	return new RouterSettings(*this);
}

void Router::update() {
	Entity& en = Entity::get(id);
	alert.notice = 0;
	alert.warning = 0;
	alert.critical = 0;

	if (en.isGhost()) return;
	if (!en.isEnabled()) return;

	for (auto& rule: rules) {
		rule.icon = std::max(1, std::min((int)Sim::customIcons.size()-1, rule.icon));
		rule.nicSrc = std::max(0, std::min((int)networker->interfaces.size()-1, rule.nicSrc));
		rule.nicDst = std::max(0, std::min((int)networker->interfaces.size()-1, rule.nicDst));

		if (!rule.condition.valid()) continue;

		// meta signals can still be evaluated when nothing else is present
		minimap<Signal,&Signal::key> signals;
		if (networker->interfaces[rule.nicSrc].network) signals = networker->interfaces[rule.nicSrc].network->signals;

		switch (rule.mode) {
			case Rule::Mode::Forward: {
				if (!networker->interfaces[rule.nicDst].network) continue;
				for (auto& signal: signals) {
					if (rule.condition.evaluateNotNull(signal))
						networker->interfaces[rule.nicDst].write(signal);
				}
				break;
			}
			case Rule::Mode::Generate: {
				if (!rule.signal.valid()) break;
				if (!networker->interfaces[rule.nicDst].network) continue;
				if (rule.condition.evaluate(signals)) {
					networker->interfaces[rule.nicDst].write(rule.signal);
				}
				break;
			}
			case Rule::Mode::Alert: {
				if (rule.condition.evaluate(signals)) {
					if (rule.alert == Rule::Alert::Notice) {
						Sim::alerts.customNotice++;
						alert.notice = rule.icon;
					}
					if (rule.alert == Rule::Alert::Warning) {
						Sim::alerts.customWarning++;
						alert.warning = rule.icon;
					}
					if (rule.alert == Rule::Alert::Critical) {
						Sim::alerts.customCritical++;
						alert.critical = rule.icon;
					}
				}
				break;
			}
		}
	}
}

