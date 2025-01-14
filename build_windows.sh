#!/bin/bash

# İçinde bulunulan klasörün adını alın
current_dir=$(basename "$PWD")

# Eğer klasör adı "Pcontum3D" ise, "bin" ve "out" klasörlerini sil
if [ "$current_dir" = "LearnOpenGL" ]; then
    # "bin" klasörünü kontrol edip sil
    if [ -d "bin" ]; then
        rm -rf bin
        echo "bin klasörü başarıyla silindi."
    else
        echo "bin klasörü bulunamadı."
    fi
    
    # "out" klasörünü kontrol edip sil
    if [ -d "out" ]; then
        rm -rf out
        echo "out klasörü başarıyla silindi."
    else
        echo "out klasörü bulunamadı."
    fi
else
    echo "Bu komut yalnızca 'LearnOpenGL' klasöründe çalıştırılabilir."
fi

mkdir -p out
cd out

cmake -G"Visual Studio 17 2022" ${COMMON_CMAKE_CONFIG_PARAMS} ../
cmake --build . --config Debug


echo "-----------------------------"

echo "-----------------------------"

echo "-----------------------------"

echo "Press <Enter> to exit "
read -r
