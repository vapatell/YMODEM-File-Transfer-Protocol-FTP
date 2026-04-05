################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Ensc351Part2.cpp \
../Medium.cpp \
../PeerY.cpp \
../ReceiverY.cpp \
../SenderY.cpp \
../main.cpp \
../myIO.cpp 

OBJS += \
./Ensc351Part2.o \
./Medium.o \
./PeerY.o \
./ReceiverY.o \
./SenderY.o \
./main.o \
./myIO.o 

CPP_DEPS += \
./Ensc351Part2.d \
./Medium.d \
./PeerY.d \
./ReceiverY.d \
./SenderY.d \
./main.d \
./myIO.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++17 -I"/home/osboxes/git/ensc351lib2/Ensc351" -I"/mnt/hgfs/VMsf2020/eclipse-workspace-2021-06/Ensc351Part2CodedBySmartState/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

PeerY.o: ../PeerY.cpp subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++17 -I"/home/osboxes/git/ensc351lib2/Ensc351" -I"/mnt/hgfs/VMsf2020/eclipse-workspace-2021-06/Ensc351Part2CodedBySmartState/src" -O0 -g3 -Wall -c -fmessage-length=0 -Wno-register -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


