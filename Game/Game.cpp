#include "opencv2/opencv.hpp"
#include <iostream>
#include <ctime>
#include <vector>
#include "SFML/Audio.hpp"

// =====================
// 이미지 오브젝트 구조체
// =====================
struct ImageObject {
    cv::Point position;
    int radius;
    bool active;
    int ballPoint;

    ImageObject() {	// 생성자
		this->position = cv::Point();
		this->radius = 0;
		this->active = false;
        this->ballPoint = 1;
	}
};

// =====================
// 랜덤 위치
// =====================
cv::Point getRandomPosition(int width, int height, int radius) {
    int x = rand() % (width - 2 * radius) + radius;
    int y = rand() % (height - 2 * radius) + radius;
    return cv::Point(x, y);
}

// =====================
// 공 점수 랜덤 설정 (5번 중 1번은 3점)
// =====================
int getRandomPoint() {
    if (rand() % 5 == 0)
        return 3;   // 보너스
    return 1;       // 기본
}

// =====================
// 악마 뿔 그리기 (원 밖으로)
// =====================
void drawDevilHorns(cv::Mat& frame, cv::Point center, int radius,int score) {
    cv::Scalar hornColor;

    if (score >= 40) {
        hornColor = cv::Scalar(0, 0, 255);       // 진한 Red (최종 단계)
    }
    else if (score >= 30) {
        hornColor = cv::Scalar(0, 0, 200);       // 어두운 Red
    }
    else if (score >= 20) {
        hornColor = cv::Scalar(0, 255, 255);     // Yellow
    }
    else if (score >= 10) {
        hornColor = cv::Scalar(0, 255, 0);       // Green
    }

    int hornHeight = radius;

    // 왼쪽 뿔
    std::vector<cv::Point> leftHorn = {
        cv::Point(center.x - radius / 2, center.y - radius / 2),
        cv::Point(center.x - radius / 3, center.y - radius - hornHeight),
        cv::Point(center.x, center.y - radius / 2)
    };

    // 오른쪽 뿔
    std::vector<cv::Point> rightHorn = {
        cv::Point(center.x, center.y - radius / 2),
        cv::Point(center.x + radius / 3, center.y - radius - hornHeight),
        cv::Point(center.x + radius / 2, center.y - radius / 2)
    };

    // 삼각형 그리기
    cv::fillConvexPoly(frame, leftHorn, hornColor);
    cv::fillConvexPoly(frame, rightHorn, hornColor);
}

// =====================
// 메인
// =====================
void runProject() CV_NOEXCEPT {
    srand((unsigned int)time(0));

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "웹캠이 없습니다.\n";
        return;
    }

    int width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("touch.wav")) {
        std::cout << " 사운드 파일 로드 실패\n";
        return ;
    }

    // 사운드 설정
    sf::Sound sound;
    sound.setBuffer(buffer);

    // Ball 이미지 로드
    cv::Mat ballIMG = cv::imread("dog.jpg");
    if (ballIMG.empty()) {
        std::cerr << "이미지 로드 실패\n";
        return;
    }   

    cv::Mat prevGray;
    int score = 0;

    ImageObject obj;
    obj.radius = 40;
    obj.position = getRandomPosition(width, height, obj.radius);
    obj.ballPoint = getRandomPoint();

    // =====================
    // 동영상으로 저장
    // =====================
    double fps = cap.get(cv::CAP_PROP_FPS);
    // 코덱을 선택
    int fourcc = cv::VideoWriter::fourcc('D', 'I', 'V', 'X');
    int delay = cvRound(1000 / fps);
    // 저장 함수
    cv::VideoWriter outputVideo("output.avi", fourcc, 30, cv::Size(width, height));

    while (true) {

        cv::Mat frame, grayFrame, diff, thresh;

        cap >> frame;
        outputVideo << frame;

        if (frame.empty()) break;

        cv::flip(frame, frame, 1);

        cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(grayFrame, grayFrame, cv::Size(15, 15), 0);

        if (prevGray.empty()) {
            grayFrame.copyTo(prevGray);
            continue;
        }

        cv::absdiff(prevGray, grayFrame, diff);
        cv::threshold(diff, thresh, 25, 255, cv::THRESH_BINARY);

        if (!obj.active) {
            // 터치 감지
            // IMG Ball 외곽선 사각형
            int x1 = cv::max(0, obj.position.x - obj.radius);
            int y1 = cv::max(0, obj.position.y - obj.radius);
            int x2 = cv::min(width, obj.position.x + obj.radius);
            int y2 = cv::min(height, obj.position.y + obj.radius);

            cv::Rect roiRect(x1, y1, x2 - x1, y2 - y1);
            cv::Mat roi(thresh, roiRect);

            int movement = cv::countNonZero(roi);

            int area = (obj.radius * 2) * (obj.radius * 2);

            if (movement > area * 0.1) {
                score += obj.ballPoint;
                sound.play();
                // 새 공 생성
                obj.position = getRandomPosition(width, height, obj.radius);
                obj.ballPoint = getRandomPoint();

            }
        }

        /* =========================
*          점수가 커질수록 Ball이 작아지도록 기본 사이즈 40
           점수 10 이상 → 사이즈 35
           점수 20 이상 → 사이즈 30
           점수 30 이상 → 이미지 색상 반전, 사이즈 25
           점수 40 이상 → 이미지 색상 붉은 계열로 변경, 사이즈 20
         ========================= */
        if (score >= 40) {
            obj.radius = 20;
        }
        else if (score >= 30) {
            obj.radius = 25;
        }
        else if (score >= 20) {
            obj.radius = 30;
        }
        else if (score >= 10) {
            obj.radius = 35;
        }
        else obj.radius = 40;  // 기본값

        // IMG를 obj.radius 크기로 resize
        cv::Mat ballIMGResize;
        cv::resize(ballIMG, ballIMGResize, cv::Size(obj.radius * 2, obj.radius * 2));

        // HSV 변환
        cv::Mat hsv;
        cv::cvtColor(ballIMGResize, hsv, cv::COLOR_BGR2HSV);

        // 채널 분리
        std::vector<cv::Mat> channels;
        cv::split(hsv, channels);

        // 명도 증가
        channels[2] += 50.0;

        // HSV 채널 병합 후 BGR로 변환
        cv::merge(channels, hsv);
        cv::cvtColor(hsv, ballIMGResize, cv::COLOR_HSV2BGR);

        // 붉은색 overlay
        if (score >= 40) {
            cv::Mat redOverlay(ballIMGResize.size(), ballIMGResize.type(), cv::Scalar(0, 0, 255));
            double alpha = 0.5; // 붉은 색 강도 (0.0~1.0)
            cv::addWeighted(redOverlay, alpha, ballIMGResize, 1 - alpha, 0, ballIMGResize);
        } else if (score >= 30) { // 색 반전
            ballIMGResize = ~ballIMGResize; 
        }

        cv::Mat mask(ballIMGResize.size(), CV_8UC1, cv::Scalar(0));
        cv::circle(mask, cv::Point(obj.radius, obj.radius), obj.radius, cv::Scalar(255, 255, 255), -1);

        // IMG Ball 외곽선 사각형
        int x1 = cv::max(0, obj.position.x - obj.radius);
        int y1 = cv::max(0, obj.position.y - obj.radius);
        int x2 = cv::min(width, obj.position.x + obj.radius);
        int y2 = cv::min(height, obj.position.y + obj.radius);

        cv::Rect dstRect(x1, y1, x2 - x1, y2 - y1);

        ballIMGResize.copyTo(frame(dstRect), mask);

        // =====================
        // 10점 이상이면 뿔
        // =====================
        if (score >= 10) {
            drawDevilHorns(frame, obj.position, obj.radius, score);
        }

        // =====================
        // 공 옆에 점수 표시 (가독성 개선)
        // =====================

        // 텍스트 위치: 공 오른쪽 위
        cv::Point textPos(
            obj.position.x + obj.radius + 8,
            obj.position.y - obj.radius / 2
        );

        // 색상 결정
        cv::Scalar textColor =
            (obj.ballPoint == 3)
            ? cv::Scalar(0, 255, 255)   // +3 → 노랑
            : cv::Scalar(255, 255, 255); // +1 → 흰색

        // 그림자(검정) 먼저
        cv::putText(frame, "+" + std::to_string(obj.ballPoint), textPos + cv::Point(2, 2), cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(0, 0, 0),  3 );

        // 실제 텍스트
        cv::putText( frame, "+" + std::to_string(obj.ballPoint), textPos, cv::FONT_HERSHEY_SIMPLEX, 0.9, textColor, 2 );

        // UI
        cv::putText(frame, "Score : " + std::to_string(score), cv::Point(20, 40)
            , cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(255, 255, 255), 2);

        cv::imshow("GAME", frame);
        grayFrame.copyTo(prevGray);

        if (cv::waitKey(10) == 27) {
            break;
        }
    }

    // =====================
    // 게임 종료 화면
    // =====================

    // 프레임 크기와 동일한 검은 배경
    cv::Mat gameOver(height, width, CV_8UC3, cv::Scalar(0, 0, 0));

    // =====================
    // 점수 기반 공 개수 계산
    // =====================
    // 예: score = 73 → 7개
    int ballCount = score / 10;

    // =====================
    // 종료 화면에 뿌릴 공 생성
    // =====================
    int centerX = width / 2;
    int centerY = height / 2;
    int centerRadius = 80;  // 회전 공 반지름

    for (int i = 0; i < ballCount; i++) {

        int radius = 20;

        // 랜덤 위치( 중앙의 이미지와 겹치지 않게)
        cv::Point pos;
        while (true) {
            pos = getRandomPosition(width, height, radius);
            int dx = pos.x - centerX;
            int dy = pos.y - centerY;
            if (sqrt(dx * dx + dy * dy) > radius + centerRadius + 5)
                break;
        }

        // 공 이미지 resize
        cv::Mat endBall;
        cv::resize(ballIMG, endBall, cv::Size(radius * 2, radius * 2));

        // 원형 마스크 생성
        cv::Mat maskEnd(endBall.size(), CV_8UC1, cv::Scalar(0));
        cv::circle(maskEnd, cv::Point(radius, radius), radius, cv::Scalar(255, 255, 255), -1);

        // 화면 밖으로 나가지 않게 보정
        int x1 = cv::max(0, pos.x - radius);
        int y1 = cv::max(0, pos.y - radius);
        int x2 = cv::min(width, pos.x + radius);
        int y2 = cv::min(height, pos.y + radius);

        cv::Rect roi(x1, y1, x2 - x1, y2 - y1);

        // 공 합성
        endBall.copyTo(gameOver(roi), maskEnd);
    }

    // 원형 이미지 크기 조정
    int endRadius = 80;
    cv::Mat endImg;
    cv::resize(ballIMG, endImg, cv::Size(endRadius * 2, endRadius * 2));

    // 회전 행렬(원형 이미지의 회전)
    cv::Point2f center(endImg.cols / 2.0f, endImg.rows / 2.0f);
    cv::Mat rotMat = cv::getRotationMatrix2D(center, -40.0, 1.0);

    // 회전 적용
    cv::Mat rotated;
    cv::warpAffine(endImg, rotated, rotMat, endImg.size());

    // 원형 마스크
    cv::Mat mask(rotated.size(), CV_8UC1, cv::Scalar(0));
    cv::circle(mask, cv::Point(endRadius, endRadius), endRadius, cv::Scalar(255, 255, 255), -1);

    // 중앙 위치
    int cx = width / 2 - endRadius;
    int cy = height / 2 - endRadius;
    cv::Rect roi(cx, cy, endRadius * 2, endRadius * 2);

    // 합성
    rotated.copyTo(gameOver(roi), mask);

    // 텍스트
    cv::putText(gameOver, "GAME OVER", cv::Point(width / 2 - 150, height / 2 + 140), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 255), 3);

    cv::putText(gameOver, "Final Score : " + std::to_string(score), cv::Point(width / 2 - 135, height / 2 + 190), cv::FONT_HERSHEY_SIMPLEX, 1.0,  cv::Scalar(255, 255, 255), 2);

    // 같은 창 이름 사용
    cv::imshow("GAME", gameOver);

    cap.release();

    cv::waitKey(0);
    cv::destroyAllWindows();
}
