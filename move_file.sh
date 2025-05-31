#!/bin/bash

# 파일 이동 및 include 경로 자동 업데이트 스크립트
# 사용법: ./move_file.sh 원본_파일_경로 대상_파일_경로

if [ $# -ne 2 ]; then
    echo "사용법: $0 원본_파일_경로 대상_파일_경로"
    exit 1
fi

SOURCE_PATH=$1
TARGET_PATH=$2
PROJECT_ROOT=$(pwd)

# 상대 경로를 절대 경로로 변환
if [[ "$SOURCE_PATH" != /* ]]; then
    SOURCE_PATH="$PROJECT_ROOT/$SOURCE_PATH"
fi

if [[ "$TARGET_PATH" != /* ]]; then
    TARGET_PATH="$PROJECT_ROOT/$TARGET_PATH"
fi

# 파일이 존재하는지 확인
if [ ! -f "$SOURCE_PATH" ]; then
    echo "오류: 원본 파일 '$SOURCE_PATH'이(가) 존재하지 않습니다."
    exit 1
fi

# 대상 디렉터리가 존재하는지 확인하고, 없으면 생성
TARGET_DIR=$(dirname "$TARGET_PATH")
if [ ! -d "$TARGET_DIR" ]; then
    echo "대상 디렉터리 '$TARGET_DIR'을(를) 생성합니다."
    mkdir -p "$TARGET_DIR"
fi

# 파일 이름과 확장자 추출
SOURCE_FILENAME=$(basename "$SOURCE_PATH")
SOURCE_EXT="${SOURCE_FILENAME##*.}"
SOURCE_NAME="${SOURCE_FILENAME%.*}"

# 원본 파일의 include 경로 형식 추출 (C/C++ 파일에만 적용)
if [[ "$SOURCE_EXT" == "c" || "$SOURCE_EXT" == "cpp" || "$SOURCE_EXT" == "h" || "$SOURCE_EXT" == "hpp" ]]; then
    # 파일에서 include 경로 추출
    INCLUDE_PATHS=$(grep -o '#include "[^"]*"' "$SOURCE_PATH" | sed 's/#include "//g' | sed 's/"//g')
    
    # 파일 내에서 자신의 경로를 참조하는 경우 업데이트
    SOURCE_REL_PATH=$(realpath --relative-to="$PROJECT_ROOT" "$SOURCE_PATH")
    TARGET_REL_PATH=$(realpath --relative-to="$PROJECT_ROOT" "$TARGET_PATH")
    
    # 파일 복사 및 경로 업데이트
    echo "파일을 '$SOURCE_PATH'에서 '$TARGET_PATH'로 복사합니다."
    cp "$SOURCE_PATH" "$TARGET_PATH"
    
    # 대상 파일 내의 경로 업데이트
    if [ -n "$INCLUDE_PATHS" ]; then
        echo "대상 파일 내의 include 경로를 업데이트합니다..."
        
        # 원본 파일의 디렉터리와 대상 파일의 디렉터리
        SOURCE_DIR=$(dirname "$SOURCE_PATH")
        
        for INCLUDE_PATH in $INCLUDE_PATHS; do
            # 절대 경로인 경우 무시
            if [[ "$INCLUDE_PATH" == /* ]]; then
                continue
            fi
            
            # 상대 경로 계산
            ABSOLUTE_INCLUDE="$SOURCE_DIR/$INCLUDE_PATH"
            if [ -f "$ABSOLUTE_INCLUDE" ]; then
                NEW_REL_PATH=$(realpath --relative-to="$TARGET_DIR" "$ABSOLUTE_INCLUDE")
                echo "  $INCLUDE_PATH -> $NEW_REL_PATH"
                sed -i '' "s|#include \"$INCLUDE_PATH\"|#include \"$NEW_REL_PATH\"|g" "$TARGET_PATH"
            fi
        done
    fi
    
    # 프로젝트 내 모든 파일에서 원본 파일을 참조하는 include 문 업데이트
    echo "프로젝트 내 파일들의 include 경로를 업데이트합니다..."
    
    # 찾을 파일 확장자 목록
    FILE_EXTS=("c" "cpp" "h" "hpp")
    
    for EXT in "${FILE_EXTS[@]}"; do
        FILES=$(find "$PROJECT_ROOT" -name "*.$EXT" -type f)
        for FILE in $FILES; do
            if [ "$FILE" != "$TARGET_PATH" ]; then  # 대상 파일 자체는 제외
                FILE_DIR=$(dirname "$FILE")
                SOURCE_REL_FROM_FILE=$(realpath --relative-to="$FILE_DIR" "$SOURCE_PATH" 2>/dev/null || echo "")
                
                if grep -q "#include \".*$SOURCE_FILENAME\"" "$FILE"; then
                    TARGET_REL_FROM_FILE=$(realpath --relative-to="$FILE_DIR" "$TARGET_PATH")
                    echo "  업데이트: $FILE"
                    # 원본 파일명을 포함하는 include 문을 찾아 새 경로로 업데이트
                    sed -i '' "s|#include \".*$SOURCE_FILENAME\"|#include \"$TARGET_REL_FROM_FILE\"|g" "$FILE"
                fi
            fi
        done
    done
    
    # Makefile 업데이트 (특정 패턴에 맞는 경우)
    MAKEFILES=$(find "$PROJECT_ROOT" -name "Makefile" -type f)
    for MAKEFILE in $MAKEFILES; do
        SOURCE_REL_FROM_ROOT=$(realpath --relative-to="$PROJECT_ROOT" "$SOURCE_PATH")
        TARGET_REL_FROM_ROOT=$(realpath --relative-to="$PROJECT_ROOT" "$TARGET_PATH")
        
        if grep -q "$SOURCE_REL_FROM_ROOT" "$MAKEFILE"; then
            echo "  Makefile 업데이트: $MAKEFILE"
            sed -i '' "s|$SOURCE_REL_FROM_ROOT|$TARGET_REL_FROM_ROOT|g" "$MAKEFILE"
        fi
    done
else
    # 비 C/C++ 파일은 단순 이동
    echo "파일을 '$SOURCE_PATH'에서 '$TARGET_PATH'로 이동합니다."
    mv "$SOURCE_PATH" "$TARGET_PATH"
fi

echo "완료!"
