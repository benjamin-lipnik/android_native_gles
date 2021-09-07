SDK_PATH=~/Android/Sdk
BUILD_TOOLS_PATH=$(SDK_PATH)/build-tools/30.0.3
NDK_BUILD=$(SDK_PATH)/ndk/22.1.7171670/ndk-build
AAPT=$(BUILD_TOOLS_PATH)/aapt
ANDROID_VERSION=24
PACKAGE_NAME=eu.benjaminlipnik.NativeExample
ADB=adb

all: build package
	
build:
	$(NDK_BUILD)

STOREPASS=password
DNAME="CN=example.com, OU=ID, O=Example, L=Doe, S=John, C=GB"
KEYSTOREFILE=my-release-key.keystore
ALIASNAME=standkey

keystore : $(KEYSTOREFILE)

$(KEYSTOREFILE) :
	keytool -genkey -v -keystore $(KEYSTOREFILE) -alias $(ALIASNAME) -keyalg RSA -keysize 2048 -validity 10000 -storepass $(STOREPASS) -keypass $(STOREPASS) -dname $(DNAME)

package: AndroidManifest.xml
	rm -rf apkbuilder result.apk
	mkdir apkbuilder
	mkdir apkbuilder/makecapk/
	
	cp -r libs/ apkbuilder/makecapk/lib/
	$(AAPT) package -f -F apkbuilder/temp.apk -I $(SDK_PATH)/platforms/android-$(ANDROID_VERSION)/android.jar -M AndroidManifest.xml -v --target-sdk-version $(ANDROID_VERSION)
	unzip -o apkbuilder/temp.apk -d apkbuilder/makecapk
	cd apkbuilder/makecapk && zip -D9r ../makecapk.apk . && zip -D0r ../makecapk.apk ./AndroidManifest.xml
	jarsigner -sigalg SHA1withRSA -digestalg SHA1 -verbose -keystore $(KEYSTOREFILE) -storepass $(STOREPASS) apkbuilder/makecapk.apk $(ALIASNAME)
	$(BUILD_TOOLS_PATH)/zipalign -v 4 apkbuilder/makecapk.apk result.apk

clean:
	rm -rf obj/ libs/ result.apk

uninstall : 
	($(ADB) uninstall $(PACKAGE_NAME))||true

push : build package
	@echo "Installing" $(PACKAGE_NAME)
	$(ADB) install -r result.apk

run : push
	$(eval ACTIVITYNAME:=$(shell $(AAPT) dump badging result.apk | grep "launchable-activity" | cut -f 2 -d"'"))
	$(ADB) shell am start -n $(PACKAGE_NAME)/$(ACTIVITYNAME)

