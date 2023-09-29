/**
 * @file
 */

#include "voxelformat/private/slab6/KV6Format.h"
#include "AbstractVoxFormatTest.h"
#include "voxelformat/private/slab6/KVXFormat.h"

namespace voxelformat {

class KV6FormatTest : public AbstractVoxFormatTest {
protected:
	static core::SharedPtr<voxel::RawVolume> createCubeModel() {
		glm::ivec3 mins(0, 0, 0);
		glm::ivec3 maxs(9, 9, 9);
		voxel::Region region(mins, maxs);
		core::SharedPtr<voxel::RawVolume> v = core::make_shared<voxel::RawVolume>(region);
		v->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(8, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(0, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(0, 8, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 8, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(0, 9, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 9, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(8, 9, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 9, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(0, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 0, 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(0, 9, 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 9, 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(0, 0, 8, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 0, 8, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(0, 9, 8, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 9, 8, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(0, 0, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 0, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(8, 0, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 0, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(0, 1, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 1, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(0, 8, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 8, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(0, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		v->setVoxel(1, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(8, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		v->setVoxel(9, 9, 9, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		return v;
	}
};

TEST_F(KV6FormatTest, testLoad) {
	canLoad("test.kv6");
}

TEST_F(KV6FormatTest, testSaveCubeModel) {
	KV6Format f;
	const core::SharedPtr<voxel::RawVolume> &model = createCubeModel();
	testSaveLoadVolume("kv6-savecubemodel.kv6", *model.get(), &f);
}

TEST_F(KV6FormatTest, testSaveSmallVoxel) {
	KV6Format f;
	testSaveLoadVoxel("kv6-smallvolumesavetest.kv6", &f, -16, 15);
}

TEST_F(KV6FormatTest, testLoadSave) {
	KV6Format f;
	testLoadSaveAndLoad("voxlap5.kv6", f, "kv6-voxlap5.kv6", f);
}

TEST_F(KV6FormatTest, DISABLED_testChrKnight) {
	KV6Format f1;
	KVXFormat f2;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Pivot;
	testLoadSceneGraph("slab6_vox_test.kv6", f1, "slab6_vox_test.kvx", f2, flags);
}

} // namespace voxelformat
