# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/turco/esp/esp-idf/components/bootloader/subproject"
  "D:/Session_6/Projet/EcouSol/Prog_Esp-32/test_I2S_GPT/build/bootloader"
  "D:/Session_6/Projet/EcouSol/Prog_Esp-32/test_I2S_GPT/build/bootloader-prefix"
  "D:/Session_6/Projet/EcouSol/Prog_Esp-32/test_I2S_GPT/build/bootloader-prefix/tmp"
  "D:/Session_6/Projet/EcouSol/Prog_Esp-32/test_I2S_GPT/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Session_6/Projet/EcouSol/Prog_Esp-32/test_I2S_GPT/build/bootloader-prefix/src"
  "D:/Session_6/Projet/EcouSol/Prog_Esp-32/test_I2S_GPT/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Session_6/Projet/EcouSol/Prog_Esp-32/test_I2S_GPT/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Session_6/Projet/EcouSol/Prog_Esp-32/test_I2S_GPT/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
