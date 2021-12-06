#ifndef DEVICE_CREDENTIALS
#define DEVICE_CREDENTIALS
// Fill in  your WiFi networks SSID and password
#define SECRET_SSID "Trap House v2"
#define SECRET_PASS "1sweetn1sourchicken4me"

// Fill in the hostname of your AWS IoT broker
#define SECRET_BROKER "a21pvb772khs3n-ats.iot.us-west-2.amazonaws.com"

// Fill in the boards public certificate
const char SECRET_CERTIFICATE[] = R"(
-----BEGIN CERTIFICATE-----
MIIC0TCCAbmgAwIBAgIUA9ineyDodLTjUMKNXjSDoF05iuwwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIxMTIwMTIyNTkx
OVoXDTQ5MTIzMTIzNTk1OVowYTELMAkGA1UEBhMCVVMxCzAJBgNVBAgTAkNBMRIw
EAYDVQQHEwlTbm9ob21pc2gxEDAOBgNVBAoTB1ByaXZhdGUxEDAOBgNVBAsTB1By
aXZhdGUxDTALBgNVBAMTBFRvbnkwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATa
AYxeREhLjYsd1fGPV3RL8j16gkgEvsA6TvZowm5NtI1x3DTZaKKzevISBhL+tUxU
BtF80aECtXXV0IPmS/4qo2AwXjAfBgNVHSMEGDAWgBS62CLJvl/4fx6unXIdD/39
RaaMUDAdBgNVHQ4EFgQUVXvqyOMIGEeuACpt/6QgtYpTlXIwDAYDVR0TAQH/BAIw
ADAOBgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQELBQADggEBADSvyEGMp1dKZlJz
78e4X7x1wO9KFftEo0ekDckcC43GrP24waFLtBntJuR8KIyS1XvE4Wt3MmMdrgnz
6YuDxwGY958E5BRzxngnktuyyJT/gGg6ux+aIbYx5Wg+nqCXvujPQm0hxKtarNKQ
N51MB2WaJBE8i25g+2cNhCFMXV4fPq1is/R5pdKaSRQGTpj70Jz5tfIZd+fdbojI
KMzoAOFgCFKUTRFZFJi3zFXaVvgdYZaCDVmFrktmQSoVfGn/fMlTLqTtf82bX27S
X02zYP+TL2i0i8R40RJjhh2kQcHWQFfseaQHdfQNCQvODZIg4fj7suwN5DguKXFJ
/391uYQ=
-----END CERTIFICATE-----
)";
#endif