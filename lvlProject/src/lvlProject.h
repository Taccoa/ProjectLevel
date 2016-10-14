#ifndef lvlProject_H_
#define lvlProject_H_

#include "gameplay.h"

using namespace gameplay;


class lvlProject: public Game
{
public:
    lvlProject();
	void keyEvent(Keyboard::KeyEvent evt, int key);
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

protected:
    void initialize();
    void finalize();
    void update(float elapsedTime);
    void render(float elapsedTime);

private:
    bool drawScene(Node* node);
    Scene* _scene;
    bool _wireframe;
	enum MessageType 
	{
		mNewMesh,
		mVertexChange,
		mNewMaterial,
		mMeshChangedMaterial,
		mTransform,
		mCamera,
		mCameraChanged,
		mLight,
		mNodeRemoved
	};
};

#endif
