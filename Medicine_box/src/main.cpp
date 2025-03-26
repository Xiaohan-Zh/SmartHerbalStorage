#include <ESP32Servo.h>
#include "BluetoothSerial.h"
#include "DHT.h"

// 检查蓝牙是否可用
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error 蓝牙未启用! 请在Arduino IDE中的工具菜单中启用它
#endif

// 定义引脚和变量
const int SERVO_PIN = 13;    // 舵机信号引脚
const int OPEN_ANGLE = 90;   // 开门角度
const int CLOSE_ANGLE = 0;   // 关门角度
int currentAngle = 0;        // 当前角度

// 定义DHT22传感器引脚
const int DHT_PIN1 = 14;  // 第一个DHT22
const int DHT_PIN2 = 27;  // 第二个DHT22
const int DHT_PIN3 = 26;  // 第三个DHT22
#define DHTTYPE DHT22

// 创建舵机和蓝牙对象
Servo doorServo;
BluetoothSerial SerialBT;

// 创建DHT对象
DHT dht1(DHT_PIN1, DHTTYPE);
DHT dht2(DHT_PIN2, DHTTYPE);
DHT dht3(DHT_PIN3, DHTTYPE);

// 当前选择的传感器编号（1-3）
int currentSensor = 1;

void setup() {
  // 初始化串口通信
  Serial.begin(115200);
  
  // 初始化蓝牙
  SerialBT.begin("ESP32_Door_Control"); // 蓝牙设备名称
  Serial.println("蓝牙设备已启动，可以配对了！");
  
  // 连接舵机
  ESP32PWM::allocateTimer(0);
  doorServo.setPeriodHertz(50);    // 标准50Hz舵机
  doorServo.attach(SERVO_PIN, 500, 2400);  // 移除currentAngle参数
  
  // 初始化门为关闭状态
  doorServo.write(CLOSE_ANGLE);
  delay(500); // 给舵机足够时间到达位置
  
  // 初始化DHT传感器
  dht1.begin();
  dht2.begin();
  dht3.begin();
  Serial.println("DHT22传感器已初始化");
}

// 平滑移动舵机
void moveServoSmooth(int targetAngle) {
  float currentPos = currentAngle;
  float step = 0.5;  // 增加步进值到2度
  
  if (targetAngle < currentAngle) {
    step = -0.5;
  }
  
  while (abs(currentPos - targetAngle) > 0.5) {
    currentPos += step;
    doorServo.write((int)currentPos);
    delay(20);  // 减少延时到10ms
  }
  
  doorServo.write(targetAngle);
  currentAngle = targetAngle;
}

// 读取并发送指定传感器的数据
void readAndSendDHTData(DHT& dht, int sensorNum) {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.printf("传感器%d读取失败\n", sensorNum);
    SerialBT.printf("传感器%d读取失败\n", sensorNum);
    return;
  }
  
  Serial.printf("传感器%d - 温度: %.1f°C, 湿度: %.1f%%\n", sensorNum, t, h);
  SerialBT.printf("传感器%d - 温度: %.1f°C, 湿度: %.1f%%\n", sensorNum, t, h);
}

void loop() {
  if (SerialBT.available()) {
    char command = SerialBT.read();
    
    switch (command) {
      case 'O':  // 开门
        moveServoSmooth(OPEN_ANGLE);
        Serial.println("门已开启");
        SerialBT.println("门已开启");
        break;
        
      case 'C':  // 关门
        moveServoSmooth(CLOSE_ANGLE);
        Serial.println("门已关闭");
        SerialBT.println("门已关闭");
        break;
        
      case '1':  // 选择传感器1
        currentSensor = 1;
        readAndSendDHTData(dht1, 1);
        break;
        
      case '2':  // 选择传感器2
        currentSensor = 2;
        readAndSendDHTData(dht2, 2);
        break;
        
      case '3':  // 选择传感器3
        currentSensor = 3;
        readAndSendDHTData(dht3, 3);
        break;
        
      case 'R':  // 读取当前选择的传感器数据
        switch(currentSensor) {
          case 1: readAndSendDHTData(dht1, 1); break;
          case 2: readAndSendDHTData(dht2, 2); break;
          case 3: readAndSendDHTData(dht3, 3); break;
        }
        break;
    }
  }
}
