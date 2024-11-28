/**
 * @file
 */

#pragma once

#include "palette/PaletteCache.h"
#include "ui/IMGUIApp.h"

/**
 * @brief This is a basic ui for voxconvert
 *
 * @ingroup Tools
 */
class VoxConvertUI : public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;
	core::String _output;
	core::String _sourceFile;
	core::String _voxconvertBinary = "vengi-voxconvert";
	core::String _targetFile;
	bool _targetFileExists = false;
	bool _overwriteTargetFile = false;
	palette::PaletteCache _paletteCache;

protected:
	app::AppState onConstruct() override;
	app::AppState onInit() override;
	void onRenderUI() override;

public:
	VoxConvertUI(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);
};
