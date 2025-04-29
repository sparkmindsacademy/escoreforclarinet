#include <SD.h>
#include <SPIFFS.h>
#include <M5Unified.h>

// 五线谱参数
const uint16_t startX = 20;                                 // 五线谱起始 X 坐标
const uint16_t startY = 20;                                 // 第一行五线谱的起始 Y 坐标
const uint16_t lineSpacing = 10;                            // 线条间距
const uint16_t staffLength = 280;                           // 五线谱长度
const uint16_t lineWidth = 2;                               // 线条宽度
const uint16_t noteRadius = 6;                              // 音符半径
const uint16_t stemLength = 35;                             // 音符竖线长度
const uint16_t noteSpacing = 25;                            // 音符间距
const uint16_t maxNotesPerLine = 12;                        // 每行显示12个音符
const uint16_t lineHeight = 5 * lineSpacing + lineSpacing;  // 每行五线谱的高度
const uint16_t secondLineY = startY + lineHeight + 15;      // 第二行五线谱的起始 Y 坐标
const uint16_t thirdLineY = startY + lineHeight * 2 + 30;   // 第三行五线谱的起始 Y 坐标

struct Note {
  int pitch;       // 简谱音高 1-7
  char accidental; // 升降记号 '#' 或 'b'，无记号为 ' '
  int rhythm;      // 节奏 1:全音符, 2:二分音符, 4:四分音符, 8:八分音符, 16:十六分音符
};

// 简谱到五线谱的映射（支持线间位置）
const float noteLines[] = { 0, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0 };  // 简谱 1-7 对应五线谱位置

// 示例旋律：包含音高、升降记号、节奏
Note melody[] = {
  {2, ' ', 4}, {3, ' ', 8}, {2, ' ', 16}, {2, ' ', 16},
  {2, ' ', 8}, {3, ' ', 4}, {1, ' ', 4}, {2, ' ', 8},
  {3, ' ', 8}, {2, ' ', 8}, {2, ' ', 4}, {1, ' ', 4},
  {7, ' ', 4}, {5, ' ', 8}, {3, ' ', 16}, {3, ' ', 16},
  {3, ' ', 8}, {4, ' ', 4}, {5, ' ', 4}, {3, ' ', 8},
  {2, ' ', 8}, {2, ' ', 8}, {2, ' ', 4}, {3, ' ', 4},
  {1, ' ', 8}, {6, ' ', 4}, {7, ' ', 4}, {1, ' ', 8},
  {7, ' ', 8}, {6, ' ', 8}, {7, ' ', 4}, {1, ' ', 4},
};
const int totalNotes = sizeof(melody) / sizeof(melody[0]);

// 当前显示的音符索引
int currentNoteIndex = 0;

// 绘制五线谱
void drawStaff(uint16_t y) {
  uint16_t lineColor = TFT_BLACK;
  for (int i = 0; i < 5; i++) {
    uint16_t lineY = y + i * lineSpacing;
    M5.Display.fillRect(startX, lineY, staffLength, lineWidth, lineColor);
  }
}

// 绘制音符
void drawNoteWithRhythm(const float line, int position, uint16_t y, char accidental, int rhythm) {
  uint16_t noteColor = TFT_BLACK;
  int noteX = startX + position * noteSpacing;
  int noteY = y + (4.0 - line) * lineSpacing;

  if (accidental == '#') {
    M5.Display.drawString("#", noteX - noteRadius * 3, noteY - noteRadius, 2);
  } else if (accidental == 'b') {
    M5.Display.drawString("b", noteX - noteRadius * 3, noteY - noteRadius, 2);
  }

  float angle = PI / 6;
  float cosA = cos(angle);
  float sinA = sin(angle);

  float majorRadius = noteRadius;
  float minorRadius = noteRadius * 0.7;

  bool filled = rhythm > 4;
  for (int px = -majorRadius; px <= majorRadius; px++) {
    for (int py = -minorRadius; py <= minorRadius; py++) {
      float rotatedX = px * cosA - py * sinA;
      float rotatedY = px * sinA + py * cosA;

      if (rotatedX * rotatedX / (majorRadius * majorRadius) + rotatedY * rotatedY / (minorRadius * minorRadius) <= 1) {
        if (filled) {
          M5.Display.drawPixel(noteX + px, noteY + py, noteColor);
        } else {
          if (abs(rotatedX * rotatedX / (majorRadius * majorRadius) + rotatedY * rotatedY / (minorRadius * minorRadius) - 1) < 0.5) {
            M5.Display.drawPixel(noteX + px, noteY + py, noteColor);
          }
        }
      }
    }
  }

  bool stemUp = (line < 2);
  int stemX = stemUp ? (noteX + noteRadius) : (noteX - noteRadius) + 2;
  int stemYStart = noteY;
  int stemYEnd = stemUp ? (noteY - stemLength) : (noteY + stemLength);

  // 使用 fillRect 绘制竖线
  M5.Display.fillRect(stemX - 1, stemYStart < stemYEnd ? stemYStart : stemYEnd, 2, abs(stemYEnd - stemYStart), noteColor);

  int tails = 0;
  if (rhythm == 8) tails = 1;
  else if (rhythm == 16) tails = 2;
  else if (rhythm == 32) tails = 3;

  for (int i = 0; i < tails; i++) {
    int tailYStart = stemYEnd + i * 5 * (stemUp ? 1 : -1);
    int tailYEnd = tailYStart + (stemUp ? 5 : -5);

    if (stemUp) {
      // 符杆向上时，符尾向右
      M5.Display.drawLine(stemX, tailYStart, stemX + 10, tailYEnd, noteColor);
    } else {
      // 符杆向下时，符尾向左
      M5.Display.drawLine(stemX, tailYStart, stemX - 10, tailYEnd, noteColor);
    }
  }
}

// 清除五线谱区域
void clearStaff() {
  M5.Display.fillRect(startX - noteSpacing, startY - lineSpacing, staffLength + noteSpacing * 2, lineHeight * 3 + lineSpacing * 2, TFT_WHITE);
}

// 绘制五线谱
void drawCurrentStaff() {
  clearStaff();

  // 绘制三行五线谱
  drawStaff(startY);
  drawStaff(secondLineY);
  drawStaff(thirdLineY);

    // 绘制音符
  for (int i = currentNoteIndex; i < min(currentNoteIndex + maxNotesPerLine * 3, totalNotes); i++) {
    Note currentNote = melody[i];

    int line = noteLines[currentNote.pitch - 1];
    int lineIndex = (i - currentNoteIndex) / maxNotesPerLine;
    int position = (i - currentNoteIndex) % maxNotesPerLine;

    uint16_t y;
    if (lineIndex == 0) y = startY;
    else if (lineIndex == 1) y = secondLineY;
    else y = thirdLineY;

    drawNoteWithRhythm(line, position, y, currentNote.accidental, currentNote.rhythm);
  }
}

bool pageTurn() {
  //M5.Lcd.printf("%5.2f  %5.2f  %5.2f deg", pitch, roll, yaw);

  if (roll < -20 && roll > -90) {  //前
    if (pitch > 0 && pitch < 10) {
      Serial.println("Front!");  //front
      SerialBT.println(1);
    } else if (pitch < -10 && pitch > -35) {
      Serial.println("LeftFront!");  //leftfront
      SerialBT.println(5);
    } else if (pitch > 10 && pitch < 40) {
      Serial.println("RightFront!");  //rightfront
      SerialBT.println(6);
    }
  }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // 清除屏幕
  M5.Display.fillScreen(TFT_WHITE);
  // 绘制初始五线谱
  drawCurrentStaff();
}

void loop() {
  if (M5.BtnA.wasPressed()) {
    // 按钮 A 按下，向上滚动
    currentNoteIndex = (currentNoteIndex - 1 + totalNotes) % totalNotes;
    drawCurrentStaff();
  }

  if (M5.BtnB.wasPressed()) {
    // 按钮 B 按下，向下滚动
    currentNoteIndex = (currentNoteIndex + 1) % totalNotes;
    drawCurrentStaff();
  }

  M5.update();
}