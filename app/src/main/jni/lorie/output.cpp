#include <LorieImpls.hpp>
#include <lorie-compositor.hpp>
#include <lorie-client.hpp>

void LorieOutput::on_destroy() {
	LorieClient* c = LorieClient::get(client);
	if (c == nullptr) return;
	if (c->output == this) c->output = NULL;
}

void LorieOutput::on_create() {
	if (client == nullptr) return;
	if ((*client)->output != this && (*client)->output != nullptr) (*client)->output->destroy();
	(*client)->output = this;
	
	report_mode();
}

void LorieOutput::report_mode() {
	if (resource == NULL) return;
	LorieClient* c = LorieClient::get(client);
	if (c == nullptr) return;
	
	LorieRenderer& r = c->compositor.renderer;
	
	send_geometry(0, 0, r.physical_width, r.physical_height, 0, "Lorie", "none", 0);
	send_scale(1.0);
	send_mode(3, r.width, r.height, 60000);
	send_done();
}
