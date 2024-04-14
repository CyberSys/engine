/**
 * @file
 */

#include "../MementoPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void MementoPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
#if 0
	IM_REGISTER_TEST(engine, testCategory(), "none")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, title));
	};
#endif
}

} // namespace voxedit
