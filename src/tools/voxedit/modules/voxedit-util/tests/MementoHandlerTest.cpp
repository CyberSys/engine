/**
 * @file
 */

#include "../MementoHandler.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include <memory>

namespace voxedit {

class MementoHandlerTest : public app::AbstractTest {
protected:
	voxedit::MementoHandler mementoHandler;
	std::shared_ptr<voxel::RawVolume> create(int size) const {
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size - 1));
		EXPECT_EQ(size, region.getWidthInVoxels());
		return std::make_shared<voxel::RawVolume>(region);
	}
	void SetUp() override {
		ASSERT_TRUE(mementoHandler.init());
	}

	void TearDown() override {
		mementoHandler.shutdown();
	}
};

TEST_F(MementoHandlerTest, testMarkUndo) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	std::shared_ptr<voxel::RawVolume> third = create(3);
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_FALSE(mementoHandler.canUndo());

	mementoHandler.markUndo(0, 0, "", first.get());
	EXPECT_FALSE(mementoHandler.canRedo())
		<< "Without a second entry and without undoing something before, you can't redo anything";
	EXPECT_FALSE(mementoHandler.canUndo())
		<< "Without a second entry, you can't undo anything, because it is your initial state";
	EXPECT_EQ(1, (int)mementoHandler.stateSize());
	EXPECT_EQ(0, (int)mementoHandler.statePosition());

	mementoHandler.markUndo(0, 0, "", second.get());
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(2, (int)mementoHandler.stateSize());
	EXPECT_EQ(1, (int)mementoHandler.statePosition());

	mementoHandler.markUndo(0, 0, "", third.get());
	EXPECT_FALSE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, (int)mementoHandler.statePosition());
}

TEST_F(MementoHandlerTest, testUndoRedo) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	std::shared_ptr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "", first.get());
	mementoHandler.markUndo(0, 0, "", second.get());
	mementoHandler.markUndo(0, 0, "", third.get());

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());

	const voxedit::MementoState &undoThird = mementoHandler.undo();
	ASSERT_TRUE(undoThird.hasVolumeData());
	EXPECT_EQ(2, undoThird.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(1, (int)mementoHandler.statePosition());

	voxedit::MementoState undoSecond = mementoHandler.undo();
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());
	EXPECT_FALSE(mementoHandler.canUndo());
	EXPECT_EQ(0, (int)mementoHandler.statePosition());

	const voxedit::MementoState &redoSecond = mementoHandler.redo();
	ASSERT_TRUE(redoSecond.hasVolumeData());
	EXPECT_EQ(2, redoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_EQ(1, (int)mementoHandler.statePosition());

	undoSecond = mementoHandler.undo();
	ASSERT_TRUE(undoSecond.hasVolumeData());
	EXPECT_EQ(1, undoSecond.dataRegion().getWidthInVoxels());
	EXPECT_TRUE(mementoHandler.canRedo());
	EXPECT_FALSE(mementoHandler.canUndo());
	EXPECT_EQ(0, (int)mementoHandler.statePosition());

	const voxedit::MementoState &undoNotPossible = mementoHandler.undo();
	ASSERT_FALSE(undoNotPossible.hasVolumeData());
}

TEST_F(MementoHandlerTest, testUndoRedoDifferentNodes) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	std::shared_ptr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 0", first.get());
	mementoHandler.markNodeAdded(0, 1, "Node 1", second.get());
	mementoHandler.markNodeAdded(0, 2, "Node 2", third.get());
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());

	voxedit::MementoState state;
	{
		// undo of adding node 2
		state = mementoHandler.undo();
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo of adding node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	EXPECT_FALSE(mementoHandler.canUndo());
	EXPECT_TRUE(mementoHandler.canRedo());
	{
		// redo adding node 1
		state = mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testMaxUndoStates) {
	for (int i = 0; i < MementoHandler::MaxStates * 2; ++i) {
		auto v = create(1);
		mementoHandler.markUndo(0, i, "", v.get());
	}
	ASSERT_EQ(MementoHandler::MaxStates, (int)mementoHandler.stateSize());
}

TEST_F(MementoHandlerTest, testAddNewNode) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	std::shared_ptr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 0", first.get());
	mementoHandler.markUndo(0, 0, "Node 0 Modified", second.get());
	mementoHandler.markNodeAdded(0, 1, "Node 1", third.get());
	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());
	EXPECT_TRUE(mementoHandler.canUndo());
	EXPECT_FALSE(mementoHandler.canRedo());

	MementoState state;

	{
		// undo of adding node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
	{
		// undo modification in node 0
		state = mementoHandler.undo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		// redo modification in node 0
		state = mementoHandler.redo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo of adding node 1
		state = mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeSimple) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	mementoHandler.markUndo(0, 0, "Node 0", first.get());
	mementoHandler.markNodeAdded(0, 1, "Node 1", second.get());
	MementoState state;

	EXPECT_EQ(2, (int)mementoHandler.stateSize());
	EXPECT_EQ(1, mementoHandler.statePosition());

	{
		// undo adding node 1
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(mementoHandler.canUndo());
		EXPECT_TRUE(mementoHandler.canRedo());
	}
	{
		// redo adding node 1
		state = mementoHandler.redo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
		EXPECT_FALSE(mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testDeleteNode) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	mementoHandler.markUndo(0, 0, "Node 1", first.get());
	std::shared_ptr<voxel::RawVolume> second = create(2);
	mementoHandler.markNodeAdded(0, 1, "Node 2 Added", second.get());
	mementoHandler.markNodeRemoved(0, 1, "Node 2 Deleted", second.get());

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	MementoState state;

	{
		// undo adding node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
	{
		// redo adding node 1
		state = mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeExt) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	std::shared_ptr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 0", first.get());
	mementoHandler.markUndo(0, 0, "Node 0 Modified", second.get());
	mementoHandler.markNodeAdded(0, 1, "Node 1 Added", third.get());

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	MementoState state;

	{
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}

	{
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
	}

	{
		state = mementoHandler.redo();
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
	}

	{
		state = mementoHandler.redo();
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
	}
}

TEST_F(MementoHandlerTest, testDeleteNodeExt) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	std::shared_ptr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 1", first.get());
	mementoHandler.markUndo(0, 0, "Node 1 Modified", second.get());
	mementoHandler.markNodeAdded(0, 1, "Node 2 Added", third.get());
	mementoHandler.markNodeRemoved(0, 1, "Node 2 Deleted", third.get());

	EXPECT_EQ(4, (int)mementoHandler.stateSize());
	EXPECT_EQ(3, mementoHandler.statePosition());

	MementoState state;

	{
		// undo the deletion of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeRemoved, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		EXPECT_EQ(voxedit::MementoType::SceneNodeAdded, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}

	{
		// undo the modification of node 0
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ(voxedit::MementoType::Modification, state.type);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(1, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(mementoHandler.canUndo());
	}

	{
		// redo the modification of node 0
		state = mementoHandler.redo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(0, state.nodeId);
		EXPECT_EQ("Node 1 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canRedo());
	}

	{
		// redo the add of node 1
		state = mementoHandler.redo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canRedo());
	}

	{
		// redo the removal of node 1
		state = mementoHandler.redo();
		EXPECT_EQ(3, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}

	{
		// redo the removal of node 1
		state = mementoHandler.redo();
		EXPECT_EQ(3, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(mementoHandler.canRedo());
	}

	{
		// undo the removal of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Deleted", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}

	{
		// undo the creation of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(mementoHandler.canUndo());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeMultiple) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	std::shared_ptr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 0", first.get());
	mementoHandler.markNodeAdded(0, 1, "Node 1 Added", second.get());
	mementoHandler.markNodeAdded(0, 2, "Node 2 Added", third.get());

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	MementoState state;

	{
		// undo the creation of node 2
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_TRUE(mementoHandler.canUndo());
	}
	{
		// undo the creation of node 1
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(mementoHandler.canUndo());
	}
	{
		// redo the creation of node 1
		state = mementoHandler.redo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 1 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canRedo());
	}
	{
		// redo the creation of node 2
		state = mementoHandler.redo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(2, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(mementoHandler.canRedo());
	}
}

TEST_F(MementoHandlerTest, testAddNewNodeEdit) {
	std::shared_ptr<voxel::RawVolume> first = create(1);
	std::shared_ptr<voxel::RawVolume> second = create(2);
	std::shared_ptr<voxel::RawVolume> third = create(3);
	mementoHandler.markUndo(0, 0, "Node 1", first.get());
	mementoHandler.markNodeAdded(0, 1, "Node 2 Added", second.get());
	mementoHandler.markUndo(0, 1, "Node 2 Modified", third.get());

	EXPECT_EQ(3, (int)mementoHandler.stateSize());
	EXPECT_EQ(2, mementoHandler.statePosition());

	MementoState state;

	{
		state = mementoHandler.undo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canUndo());
	}
	{
		state = mementoHandler.undo();
		EXPECT_EQ(0, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_FALSE(mementoHandler.canUndo());
	}
	{
		state = mementoHandler.redo();
		EXPECT_EQ(1, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Added", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(2, state.dataRegion().getWidthInVoxels());
		EXPECT_TRUE(mementoHandler.canRedo());
	}
	{
		state = mementoHandler.redo();
		EXPECT_EQ(2, mementoHandler.statePosition());
		EXPECT_EQ(1, state.nodeId);
		EXPECT_EQ("Node 2 Modified", state.name);
		ASSERT_TRUE(state.hasVolumeData());
		EXPECT_EQ(3, state.dataRegion().getWidthInVoxels());
		EXPECT_FALSE(mementoHandler.canRedo());
	}
}

#if 0
// TODO
TEST_F(MementoHandlerTest, testSceneNodeRenamed) {
}

TEST_F(MementoHandlerTest, testSceneNodeMove) {
}
#endif

} // namespace voxedit
