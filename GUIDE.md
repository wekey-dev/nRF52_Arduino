# MAKE ZIP

git archive --format=tar wekey | gzip >nRF52_Arduino_v21.04.24.tar.gz

# GET CHECKSUM

shasum -a 256 nRF52_Arduino_v21.04.24.tar.gz

