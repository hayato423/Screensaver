#include <windows.h>
#include <scrnsave.h>
#include <gl/gl.h>
#include <stdio.h>
#include <process.h>
#include <math.h>
#define M_PI 3.14159265358979

void EnableOpenGL(void); // prototype宣言
void DisableOpenGL(HWND); // prototype宣言
HDC hDC;
HGLRC hRC;
int wx, wy; // displayの横と縦のpixel数格納用変数
int finish = 0; /* 生成したスレッドを終了させるかどうかを最初のスレッド
		   から通知するための変数。0なら終了しない、1なら終了 */

void glCirclef(GLfloat,GLfloat,GLfloat);

void display (char * ssd)
{
  // static修飾子がついているのは、次のdisplay関数呼び出し時に前の値を使うため。
  static int i=0; // URDL100文字格納配列のインデックス
  static char direction; // 正方形が動く方向（URDLのどれか）を格納するための変数
  static double locx = 0.0; // 正方形を描画する場所のx座標
  static double locy = 0.0; // 正方形を描画する場所のy座標

  i=i % 100; // URDL100文字格納配列のindexを0から99の範囲で動かすため
  direction = ssd[i]; // i番目の文字をdirectionに代入
  i++; // 次のdisplay関数呼び出し時のためにiの値を増やす
  switch (direction) {
  case 'U':
    locy+=0.5;
    break;
  case 'D':
    locy-=0.5;
    break;
  case 'R':
    locx+=0.5;
    break;
  case 'L':
    locx-=0.5;
    break;
  default: // UDRL以外の文字が含まれていたら終了
    fprintf(stderr, "Invalid character.\n");
    exit(0);
  }
  glClear(GL_COLOR_BUFFER_BIT); // 画面全体をglClearColorで指定した色で塗る
  //glRectf(locx-20.0, locy-20.0, locx, locy); // 長方形（正方形）
  glCirclef(locx,locy,10.0);
}

void glCirclef(GLfloat locx,GLfloat locy,GLfloat radius) {
  int n = 100;
  double rate;
  GLfloat x,y;
  glBegin(GL_POLYGON);    //ポリゴンの描画
  //円を描画
  for(int i = 0; i <n; i++) {
    //座標計算
    rate = (double)i / n;
    x = radius * cos(2.0 * M_PI * rate) + locx;
    y = radius * sin(2.0 * M_PI * rate) + locy;
    glVertex3d(x,y,0.0);
  }
  glEnd();
}

unsigned __stdcall disp (void *arg) {
  char ssd[100];
  FILE *fp=NULL;

  EnableOpenGL(); // OpenGL設定

  /* OpenGL初期設定 */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-50.0*wx/wy, 50.0*wx/wy, -50.0, 50.0, -50.0, 50.0); // 座標系設定

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0.1f, 0.2f, 0.3f, 0.0f); // glClearするときの色設定
  glColor3f(0.9, 0.8, 0.7); // glRectf等で描画するときの色設定
  glViewport(0,0,wx,wy);
  /* ここまで */

  /* スクリーンセーバ記述プログラム読み込み */
  fp=fopen ("sample1", "r");
  if (fp==NULL) {
    fprintf (stderr, "failed to open the file \"sample1\"\n");
    exit(0);
  }
  for (int i=0; i<100; i++)
    if (fscanf (fp, "%c", &ssd[i]) == EOF){ // 100文字なかったら終了
      fprintf (stderr, "The length of the input is less than 100.\n");
      exit(0);
    }
  fclose(fp);
  /* ここまで */

  /* 表示関数呼び出しの無限ループ */
  while(1) {
    Sleep(15); // 0.015秒待つ
    display(ssd); // 描画関数呼び出し
    glFlush(); // 画面描画強制
    SwapBuffers(hDC); // front bufferとback bufferの入れ替え
    if (finish == 1) // finishが1なら描画スレッドを終了させる
      return 0;
  }
}

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static HANDLE thread_id1;
  RECT rc;

  switch(msg) {
  case WM_CREATE:
    hDC = GetDC(hWnd); // ウィンドウハンドルhWndからデバイスコンテクストを取得
    GetClientRect(hWnd, &rc); /* hWndのウィンドウの左上と右下の座標を構造体rcに格納する。*/
    wx = rc.right - rc.left; /* GetClientRectで取得した左上、右下の座標を使って
				displayの横のpixel数を計算 */
    wy = rc.bottom - rc.top; /* GetClientRectで取得した左上、右下の座標を使って
				displayの縦のpixel数を計算 */

    thread_id1=(HANDLE)_beginthreadex(NULL, 0, disp, NULL, 0, NULL); // 描画用スレッド生成
    if(thread_id1==0){ // スレッド生成に失敗したらスクリーンセーバを終了
      fprintf(stderr,"pthread_create : %s", strerror(errno));
      exit(0);
    }
    break;
  case WM_ERASEBKGND:
    return TRUE;
  case WM_DESTROY: // マウスやキーボードの入力があるとここに来る
    finish=1; // 描画スレッドを終了させるために1を代入
    WaitForSingleObject(thread_id1, INFINITE); // 描画スレッドの終了を待つ
    CloseHandle(thread_id1); // 生成したスレッドを閉じる
    DisableOpenGL(hWnd);
    PostQuitMessage(0); /* 自分のスレッド（メインスレッド）を終了していいことを
			   自分のメッセージキューに入れる。 */
    return 0;
  default: // 上記以外のメッセージの場合はswitch文を終了。（これは書かなくても同じだが。）
    break;
  }
  return DefScreenSaverProc(hWnd, msg, wParam, lParam); /* 上のswitch文で処理していないメッセージを処理してからreturn。*/
}

/* https://msdn.microsoft.com/ja-jp/library/cc422062.aspx 参照 */
BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return TRUE;
}

/* https://msdn.microsoft.com/ja-jp/library/cc422061.aspx 参照 */
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
  return TRUE;
}

void EnableOpenGL(void) {
  int format;
  /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd368826(v=vs.85).aspx
     を参照 */
  PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
    1,                     // version number
    0
    | PFD_DRAW_TO_WINDOW   // support window
    | PFD_SUPPORT_OPENGL   // support OpenGL
    | PFD_DOUBLEBUFFER     /* double buffered
			      (front bufferとback bufferの2枚使う) */
    ,
    PFD_TYPE_RGBA,         // RGBA type
    24,                    // 24-bit color depth
    0, 0, 0, 0, 0, 0,      // color bits ignored
    0,                     // no alpha buffer
    0,                     // shift bit ignored
    0,                     // no accumulation buffer
    0, 0, 0, 0,            // accum bits ignored
    32,                    // 32-bit z-buffer
    0,                     // no stencil buffer
    0,                     // no auxiliary buffer
    PFD_MAIN_PLANE,        // main layer
    0,                     // reserved
    0, 0, 0                // layer masks ignored
  };

  /* OpenGL設定 */
  /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd318284(v=vs.85).aspx 参照 */
  format = ChoosePixelFormat(hDC, &pfd);
  /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd369049(v=vs.85).aspx 参照 */
  SetPixelFormat(hDC, format, &pfd);
  /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd374379(v=vs.85).aspx 参照 */
  hRC = wglCreateContext(hDC);
  /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd374387(v=vs.85).aspx 参照 */
  wglMakeCurrent(hDC, hRC);
  /* ここまで */
}

void DisableOpenGL(HWND hWnd)
{
  /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd374387(v=vs.85).aspx 参照 */
  wglMakeCurrent(NULL, NULL);
  /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd374381(v=vs.85).aspx 参照 */
  wglDeleteContext(hRC);
  /* https://msdn.microsoft.com/en-us/library/windows/desktop/dd162920(v=vs.85).aspx 参照 */
  ReleaseDC(hWnd, hDC);
}
