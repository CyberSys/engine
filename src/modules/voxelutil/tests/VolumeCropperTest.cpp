/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/tests/TestHelper.h"

#include "voxelutil/VolumeCropper.h"

namespace voxelutil {

class VolumeCropperTest: public app::AbstractTest {
};

TEST_F(VolumeCropperTest, testCropSmall) {
	voxel::RawVolume smallVolume(voxel::Region(0, 2));
	smallVolume.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume *croppedVolume = voxelutil::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), glm::ivec3(0)) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), glm::ivec3(0)) << croppedRegion.toString();
	EXPECT_EQ(croppedVolume->voxel(croppedRegion.getLowerCorner()), createVoxel(voxel::VoxelType::Generic, 1));
	delete croppedVolume;
}

TEST_F(VolumeCropperTest, testCropBigger) {
	voxel::Region region = voxel::Region(0, 100);
	voxel::RawVolume smallVolume(region);
	smallVolume.setVoxel(region.getCenter(), createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume *croppedVolume = voxelutil::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_EQ(croppedVolume->voxel(region.getCenter()), createVoxel(voxel::VoxelType::Generic, 1)) << *croppedVolume;
	delete croppedVolume;
}

TEST_F(VolumeCropperTest, testCropBiggerMultiple) {
	voxel::Region region = voxel::Region(0, 100);
	voxel::RawVolume smallVolume(region);
	smallVolume.setVoxel(region.getCenter(), createVoxel(voxel::VoxelType::Generic, 1));
	smallVolume.setVoxel(region.getUpperCorner(), createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume *croppedVolume = voxelutil::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), region.getUpperCorner()) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_EQ(croppedVolume->voxel(croppedRegion.getLowerCorner()), createVoxel(voxel::VoxelType::Generic, 1)) << *croppedVolume;
	EXPECT_EQ(croppedVolume->voxel(croppedRegion.getUpperCorner()), createVoxel(voxel::VoxelType::Generic, 1)) << *croppedVolume;
	delete croppedVolume;
}

}
