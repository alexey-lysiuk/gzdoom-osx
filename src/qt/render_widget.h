
#ifndef SRC_RENDER_WIDGET_H_INCLUDED
#define SRC_RENDER_WIDGET_H_INCLUDED

#include <QtOpenGL/QGLWidget>


class RenderWidget : public QGLWidget
{
public:
	RenderWidget();
	~RenderWidget();
	
protected:
	virtual void initializeGL();
	//virtual void resizeGL( int width, int height );
	
//	virtual void resizeEvent( QResizeEvent* event );
	
	virtual void keyPressEvent( QKeyEvent* keyEvent );
    virtual void keyReleaseEvent( QKeyEvent* keyEvent );
	
private:
	void processKeyEvent( QKeyEvent* keyEvent );
	
};


extern RenderWidget* g_renderWidget;


#endif // SRC_RENDER_WIDGET_H_INCLUDED
