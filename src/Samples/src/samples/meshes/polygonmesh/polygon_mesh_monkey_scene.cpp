#include <babylon/cameras/arc_rotate_camera.h>
#include <babylon/interfaces/irenderable_scene.h>
#include <babylon/lights/point_light.h>
#include <babylon/materials/standard_material.h>
#include <babylon/math/vector2.h>
#include <babylon/meshes/mesh.h>
#include <babylon/meshes/polygonmesh/polygon.h>
#include <babylon/meshes/polygonmesh/polygon_mesh_builder.h>
#include <babylon/samples/samples_index.h>

namespace BABYLON {
namespace Samples {

struct PolygonMeshMonkeyScene : public IRenderableScene {

  PolygonMeshMonkeyScene(ICanvas* iCanvas) : IRenderableScene(iCanvas)
  {
  }

  ~PolygonMeshMonkeyScene() override = default;

  const char* getName() override
  {
    return "Polygon Mesh Monkey Scene";
  }

  void initializeScene(ICanvas* canvas, Scene* scene) override
  {
    // Create a ArcRotateCamera (non-mesh)
    auto camera = ArcRotateCamera::New("Camera", 0.f, 0.f, 50.f, Vector3::Zero(), scene);

    // Attach the camera to the canvas
    camera->attachControl(canvas, true);

    // Change the camera position
    camera->setPosition(Vector3(-20.f, 220.f, -10.f));

    // Create a basic light, aiming 0,1,0 - meaning, to the sky
    auto light = PointLight::New("light1", Vector3(20, 100, 2), scene);

    // Default intensity is 1. Let's dim the light a small amount
    light->intensity = 1.f;

    // 2D monkey shape
    std::string monkeyData
      = "-30.7428 -41.7049 -31.9015 -40.832 -32.6739 -40.444 -34.5123 -40.5501 "
        "-35.9626 -40.5436 -37.0264 -40.7339 -37.9951 -41.5062 -38.0928 -42.5706 "
        "-37.8 -43.9196 -36.3507 -44.4093 -34.8902 -44.5997 -34.5131 -45.9623 "
        "-33.1567 -46.1557 -32.081 -45.9461 -31.5143 -46.6346 -31.1211 -47.5048 "
        "-31.126 -48.8594 -30.5117 -49.5416 -30.0931 -48.512 -29.8408 -47.0594 "
        "-29.6633 -48.0291 -29.8985 -49.4199 -29.8469 -50.8722 -29.0609 -51.2922 "
        "-27.7909 -51.1142 -27.6069 -49.8435 -27.9728 -47.9686 -28.6379 -45.7908 "
        "-27.6046 -47.3017 -27.3107 -48.4528 -27.1851 -49.3605 -26.2186 -49.7207 "
        "-25.3631 -48.937 -25.1855 -47.18 -25.8537 -46.0926 -26.4529 -45.673 "
        "-26.222 -45.3106 -25.4954 -46.1571 -24.8254 -46.9365 -23.9744 -46.8823 "
        "-23.1846 -45.9709 -23.3137 -44.7669 -24.4621 -44.5277 -25.4863 -44.5161 "
        "-25.9177 -43.9741 -25.6126 -42.6428 -24.4608 -41.6775 -23.7347 -41.1323 "
        "-23.1288 -40.2858 -22.2203 -39.0157 -21.4348 -37.8046 -20.772 -36.3514 "
        "-20.4854 -34.8362 -20.1788 -33.0236 -20.1072 -29.0304 -19.9908 -26.1278 "
        "-20.4025 -24.4289 -21.0193 -23.345 -21.5604 -21.6489 -21.4991 -19.8945 "
        "-21.557 -18.0794 -21.3066 -15.7213 -20.9514 -13.6627 -20.773 -12.5735 "
        "-20.7699 -10.6377 -20.468 -9.48837 -19.7382 -8.40535 -18.9574 -7.85135 "
        "-18.0563 -7.47587 -16.8412 -6.88153 -15.5076 -6.15998 -14.1763 -5.43461 "
        "-12.9022 -5.00207 -11.9411 -5.43567 -11.5138 -6.28103 -11.4585 -7.24994 "
        "-11.2089 -8.64013 -10.9056 -10.153 -10.7902 -11.6661 -10.6087 -13.5419 "
        "-10.3656 -14.7518 -10.0651 -15.8414 -9.75352 -17.1102 -9.52007 -18.6847 "
        "-9.28164 -20.1378 -8.90539 -21.7085 -8.61497 -23.8287 -8.12303 -25.3995 "
        "-7.64123 -27.3361 -7.18957 -28.6674 -7.24708 -29.6918 -7.64695 -31.509 "
        "-8.61699 -33.685 -9.33216 -34.8417 -10.1165 -35.992 -10.9696 -37.4406 "
        "-12.1731 -39.2597 -13.2668 -41.0721 -14.1145 -42.4636 -15.5088 -44.0951 "
        "-16.4717 -45.4293 -17.0771 -46.46 -17.8067 -47.0608 -19.0161 -46.9341 "
        "-20.4686 -46.7507 -22.0444 -46.6937 -22.5213 -47.0025 -22.8815 -47.7262 "
        "-22.8927 -48.5094 -23.372 -49.0596 -24.1027 -49.4233 -23.2539 -50.4537 "
        "-21.9225 -50.9373 -20.7099 -51.3541 -19.4427 -51.906 -18.4113 -52.384 "
        "-17.6251 -52.8084 -16.776 -53.4695 -15.9321 -53.8998 -14.9051 -54.3849 "
        "-14.2952 -54.863 -13.753 -55.3509 -13.2689 -55.3497 -12.9663 -54.5027 "
        "-12.6041 -54.0115 -11.3927 -53.8995 -10.1243 -53.4735 -9.76015 -52.3247 "
        "-10.0637 -51.543 -11.2758 -51.1809 -11.9967 -50.7496 -12.7219 -50.0849 "
        "-13.0258 -49.239 -12.9044 -48.514 -12.1212 -47.603 -11.1534 -46.817 "
        "-10.4291 -45.8471 -9.75574 -45.1273 -9.1506 -44.282 -8.5458 -43.7349 "
        "-8.06811 -42.8234 -7.22182 -41.734 -6.49384 -40.7067 -5.70662 -39.6787 "
        "-5.10459 -38.8903 -4.50083 -37.9221 -3.77308 -36.4706 -3.35045 -35.7445 "
        "-2.80372 -34.5353 -2.38739 -33.6864 -1.83611 -31.995 -1.71118 -30.1802 "
        "-1.65053 -28.4256 -1.52958 -27.0339 -1.5293 -25.4606 -1.70693 -23.7646 "
        "-2.18951 -22.4325 -3.1009 -20.3168 -3.71033 -18.4425 -3.95051 -17.2926 "
        "-4.43971 -15.4788 -4.85498 -13.9033 -5.52524 -11.7266 -5.96218 -10.0364 "
        "-6.37058 -8.57986 -6.91724 -7.12857 -7.34478 -5.98087 -7.82366 -4.88951 "
        "-8.25198 -3.91841 -7.46002 -2.6517 -6.18748 -1.86782 -4.73837 -0.834822 "
        "-3.04648 0.0767719 -1.70745 0.788371 0.0431392 1.76356 1.80011 2.66568 "
        "3.24563 3.34625 5.00218 4.06908 6.39434 4.43025 8.02861 4.791 10.3275 "
        "4.66822 11.6586 4.60952 12.8681 4.42731 14.3155 3.39263 15.5896 2.42917 "
        "16.8675 1.46808 17.8306 0.314723 19.0406 -0.53289 19.8278 -1.92397 "
        "20.4824 -2.84096 21.399 -3.86138 22.1875 -5.13064 23.1443 -6.472 "
        "23.9408 -7.3097 24.6047 -8.15785 25.2695 -9.18616 25.6921 -10.3325 "
        "25.212 -11.0621 23.5782 -11.7901 22.9752 -11.8532 22.3689 -12.0918 "
        "20.8613 -12.8278 20.0075 -13.2369 19.2237 -13.4852 18.0152 -14.0951 "
        "16.3767 -14.7494 15.2282 -15.3562 14.2026 -15.7854 12.8666 -16.5033 "
        "11.7182 -17.3527 10.8141 -18.5655 9.53574 -20.0118 8.39731 -21.7128 "
        "7.48394 -23.3431 7.06224 -24.3724 6.57641 -25.6423 6.26354 -27.2762 "
        "6.32905 -29.4555 6.75744 -30.9667 7.12281 -32.1759 7.71138 -33.576 "
        "8.26694 -34.5989 8.93741 -35.7461 9.18345 -36.8342 9.53469 -38.3168 "
        "10.1418 -39.6813 10.757 -40.8869 11.2907 -42.2826 11.8365 -43.7343 "
        "12.3871 -45.1229 13.47 -46.7602 14.1404 -47.9676 14.7486 -49.175 15.354 "
        "-49.9636 15.5319 -50.6912 15.2944 -51.422 14.5593 -52.2024 14.3872 "
        "-52.8082 14.4349 -54.0826 15.1103 -55.4068 16.3106 -56.445 17.6473 "
        "-57.8926 19.1607 -58.8599 20.6114 -59.4008 21.5216 -58.7983 21.3393 "
        "-57.2858 20.1289 -56.6812 19.2242 -56.1949 18.5549 -55.2203 18.981 "
        "-55.1654 20.0706 -55.7083 21.0367 -56.1364 22.2478 -56.4339 23.7593 "
        "-56.737 25.5142 -56.6832 28.0551 -56.4987 29.3844 -56.3169 29.8045 "
        "-55.3522 29.2041 -54.0813 27.5106 -53.533 25.2108 -53.4646 22.8525 "
        "-53.6506 21.2177 -53.4764 20.0046 -53.2338 19.8267 -52.8757 21.0367 "
        "-52.878 22.5493 -52.8726 24.3041 -52.8636 25.9989 -52.9346 27.6314 "
        "-52.6259 29.203 -52.1412 30.4174 -50.9331 30.5433 -49.2327 29.9903 "
        "-48.5758 29.2023 -48.2613 27.815 -48.6364 26.4842 -49.5436 25.3913 "
        "-50.5657 24.3644 -50.9891 22.7312 -50.9891 20.854 -50.8134 20.3113 "
        "-50.3906 21.4603 -50.0877 22.8517 -50.0837 24.366 -49.9678 25.6343 "
        "-49.2986 26.533 -48.4534 26.3655 -46.9966 25.877 -46.3945 24.2416 "
        "-46.391 22.6083 -47.1188 20.7947 -48.2707 19.7056 -48.8164 18.4934 "
        "-48.7579 17.5893 -48.1488 16.7442 -46.9383 16.1969 -45.6083 15.773 "
        "-44.1565 15.4101 -42.5833 15.1028 -41.9802 14.6198 -40.83 14.1379 "
        "-39.5586 13.5343 -37.501 12.9928 -36.1687 12.6267 -35.2624 12.3826 "
        "-34.3555 12.0254 -33.3255 11.7725 -32.1803 11.2934 -31.2096 10.6292 "
        "-29.696 10.5609 -28.0621 10.6287 -26.3071 11.1064 -25.2745 11.8375 "
        "-24.1265 13.5386 -22.6826 14.8678 -21.5306 16.0731 -20.3739 17.4085 "
        "-19.7163 18.9193 -18.8058 19.9443 -18.1952 20.8539 -17.5933 22.1837 "
        "-16.7435 23.2749 -16.1424 24.4268 -15.4203 25.5762 -14.7543 26.6049 "
        "-14.0899 27.6317 -13.6011 28.7782 -13.6077 29.8105 -14.146 30.4746 "
        "-14.9945 31.6229 -15.9032 32.3546 -16.62 33.863 -17.4194 35.9204 "
        "-17.5919 37.5539 -17.4732 39.3063 -16.8658 40.5808 -15.903 41.6708 "
        "-15.1221 42.6364 -14.8171 43.9099 -14.8699 44.8106 -15.784 45.4759 "
        "-16.5088 45.9072 -17.2298 46.7569 -18.3779 47.2999 -19.2262 47.8995 "
        "-20.258 48.441 -21.1066 49.1718 -22.4957 49.8411 -23.7649 50.261 "
        "-24.8554 50.6844 -26.1258 51.3432 -27.2184 52.0128 -28.5471 52.4356 "
        "-29.8788 52.9812 -31.0879 53.2264 -32.4788 52.9292 -34.5991 52.6208 "
        "-35.5651 52.3702 -36.9537 51.8344 -38.4092 51.2848 -39.4961 50.8656 "
        "-40.5265 50.1384 -41.4935 49.4745 -42.04 48.6896 -42.6465 47.9002 "
        "-43.4299 46.9315 -43.8524 45.843 -44.5189 44.1492 -45.3683 42.9996 "
        "-45.3687 42.213 -45.3665 42.0876 -44.0362 41.8475 -42.645 41.1836 "
        "-41.1923 40.5177 -39.9227 39.6116 -39.1343 38.7646 -38.5893 37.3727 "
        "-38.2262 35.3148 -38.4068 34.0449 -38.9526 32.4121 -39.6194 31.3287 "
        "-40.5244 31.5633 -41.3744 32.2307 -42.0364 33.6215 -42.5244 34.9542 "
        "-42.5289 35.8608 -42.2824 37.0702 -41.8572 38.4589 -41.9818 39.1861 "
        "-42.5845 39.7401 -43.4291 39.9053 -44.5195 39.9179 -45.4878 39.8518 "
        "-46.2131 39.7323 -46.6372 38.8849 -45.6699 37.9769 -45.1254 37.1902 "
        "-44.3395 36.2245 -43.606 35.3764 -43.4256 34.4083 -43.3728 32.7738 "
        "-43.4885 31.5032 -43.853 30.8412 -44.3985 30.8363 -45.2489 31.7463 "
        "-45.7881 32.8948 -46.0252 34.6502 -45.8527 35.6782 -45.7909 36.4077 "
        "-46.2121 36.8297 -47.0625 36.5262 -47.3 35.5575 -47.1225 34.0451 "
        "-46.8797 32.4707 -46.8769 31.45 -47.4822 31.62 -48.0915 32.0499 "
        "-48.9918 33.9238 -49.537 35.6187 -49.4852 37.6156 -49.2303 38.2787 "
        "-49.3022 38.6923 -49.5404 38.6433 -50.3241 37.6149 -50.3287 36.4058 "
        "-50.4527 34.7122 -50.9357 33.6226 -51.5381 33.4446 -52.6205 34.4673 "
        "-53.3562 35.3761 -53.7163 36.7075 -53.7201 38.8252 -53.5349 40.0344 "
        "-52.8079 40.877 -52.1355 41.6176 -51.6582 41.6639 -52.5038 41.1828 "
        "-53.5346 41.2448 -54.4422 41.7897 -55.3445 43.1822 -55.4112 43.9703 "
        "-54.5644 44.3942 -53.7176 44.9969 -52.5063 45.7233 -50.873 46.6295 "
        "-49.4199 47.8306 -48.1356 49.9597 -46.7612 51.6502 -45.6066 52.7398 "
        "-44.7608 53.3463 -43.9755 54.1328 -43.3098 54.7386 -42.4631 55.5839 "
        "-41.252 56.1859 -39.9801 56.9151 -38.348 57.3414 -36.7753 57.5762 "
        "-35.1417 57.46 -33.3263 57.577 -31.5113 57.5276 -29.5742 57.2169 "
        "-26.9131 56.8517 -25.0383 56.2514 -23.2212 55.345 -21.8282 54.6139 "
        "-20.9858 53.8882 -20.1983 53.2313 -19.1637 52.3181 -17.897 51.3469 "
        "-16.8109 50.3185 -15.782 49.6497 -14.9994 48.8046 -14.2124 47.9622 "
        "-13.6611 47.053 -12.8161 46.2088 -12.269 45.5436 -11.422 45.1744 "
        "-10.875 45.7224 -10.4551 46.4495 -10.5712 47.5376 -10.8192 48.3231 "
        "-10.2738 48.6855 -9.6084 48.9217 -8.64071 48.9322 -7.18776 48.687 "
        "-6.28112 47.719 -5.91752 46.8114 -5.73793 46.2678 -5.73548 45.8408 "
        "-5.31462 45.4204 -4.64729 45.1139 -3.6198 44.8136 -2.46972 44.4593 "
        "-1.43788 43.9651 -0.170934 43.6077 1.10209 43.0626 2.06995 42.5227 "
        "2.98254 41.8518 3.70412 40.9408 4.48593 39.972 5.08823 38.7622 5.6293 "
        "37.5531 5.93722 36.6466 6.2463 35.0719 6.1906 33.6843 5.87382 32.3517 "
        "5.39473 31.1986 4.73315 30.5941 4.0675 29.6825 3.22382 29.0291 2.48877 "
        "28.6565 1.82921 28.2972 1.34793 27.7532 1.34296 27.2646 1.82245 26.6637 "
        "2.43065 25.6789 2.37865 25.3234 1.58272 25.5161 0.617623 26.3563 "
        "-0.839496 27.0243 -1.68382 27.5752 -2.89216 27.7543 -3.98076 28.417 "
        "-4.89004 28.7906 -5.67509 28.9059 -6.88831 28.5392 -7.49887 27.7469 "
        "-7.49471 27.2708 -6.70333 26.7229 -5.73725 25.9375 -4.46598 25.3954 "
        "-3.49674 24.8465 -2.34929 24.2963 -1.2046 23.7131 -0.402416 23.0353 "
        "0.556745 22.6693 1.34181 22.0655 2.37112 21.4656 3.10153 20.794 3.88301 "
        "20.0084 4.85178 19.5235 5.69822 18.9773 6.36267 18.4349 7.15059 17.3431 "
        "8.17513 15.7745 9.27139 14.8659 9.75335 14.0228 10.3635 13.0483 11.0803 "
        "12.3254 11.5114 11.6584 11.6933 10.4477 11.6305 9.482 11.2607 8.26731 "
        "10.8529 6.94585 10.4606 5.61033 9.98824 4.15505 9.514 2.22019 8.90516 "
        "1.1919 8.29614 -0.0199303 8.18336 -1.28849 7.75436 -2.19871 7.33577 "
        "-3.04031 6.84052 -4.25046 6.295 -5.34561 5.88582 -6.61384 5.27591 "
        "-7.58102 4.85026 -9.09478 4.24827 -10.9085 3.57986 -12.0619 2.85838 "
        "-13.3709 1.68589 -14.424 1.05319 -15.8701 0.433674 -17.4418 -0.236547 "
        "-19.135 -0.779826 -20.5311 -1.61977 -21.495 -2.29062 -22.5811 -3.2604 "
        "-23.4986 -4.34041 -24.2198 -5.07161 -25.3697 -5.68338 -26.2772 -5.67587 "
        "-27.2428 -5.06958 -27.7305 -4.22403 -27.8514 -2.71191 -28.3956 -1.74382 "
        "-29.0602 -0.896092 -29.9687 0.252322 -31.1132 1.04483 -32.1456 1.94842 "
        "-33.1665 2.50922 -34.4462 3.09519 -35.5969 3.63654 -37.1674 4.30817 "
        "-38.6206 4.97145 -40.1886 5.82808 -41.7665 6.66609 -43.3358 7.46006 "
        "-44.7305 8.17994 -45.8841 8.89912 -47.0233 9.58266 -48.1752 10.242 "
        "-49.3277 11.1459 -50.1754 11.8105 -51.3832 13.0839 -53.0191 14.4726 "
        "-53.9288 15.3176 -55.0799 16.6469 -56.2296 17.7368 -57.251 19.1335 "
        "-58.3434 20.6437 -59.3117 22.0955 -59.7364 23.0025 -60.5241 24.3332 "
        "-61.0134 25.4822 -61.2492 26.9354 -61.4966 28.5086 -61.5001 29.8999 "
        "-61.5527 31.897 -61.373 33.2889 -61.1895 34.4987 -61.0739 35.5897 "
        "-60.8013 36.4332 -60.3453 38.2508 -60.0302 39.5189 -59.802 40.974 "
        "-59.3718 42.4243 -59.0696 43.7554 -58.6505 45.1484 -58.0427 46.9627 "
        "-57.4967 48.1722 -56.7729 49.8071 -56.1624 50.7706 -54.9601 52.1099 "
        "-53.871 53.0179 -53.0793 53.6162 -52.1132 54.3449 -51.0835 55.0694 "
        "-50.1177 55.6784 -48.9671 56.2818 -47.6368 56.8886 -46.5455 57.3672 "
        "-44.9133 57.7944 -43.2201 58.646 -41.6455 59.0666 -40.2527 59.1797 "
        "-39.4067 59.3662 -37.9538 59.7225 -36.3207 59.9136 -33.9616 59.7806 "
        "-31.2976 59.6763 -29.3637 59.301 -26.8242 58.8156 -25.7314 58.3412 "
        "-24.9499 58.0887 -23.916 57.4928 -22.8936 56.8787 -21.9187 56.1612 "
        "-21.0804 55.3049 -19.9861 54.2822 -19.1371 53.3159 -17.8016 51.9866 "
        "-17.2631 50.8942 -16.4084 49.9914 -15.7495 48.838 -14.9606 47.5082 "
        "-13.9909 45.8135 -13.6308 44.0582 -13.5106 41.88 -13.6389 39.4009 "
        "-13.9966 37.8268 -14.5401 36.2534 -14.9668 35.4087 -15.6969 34.3211 "
        "-16.2975 33.1691 -16.8985 31.9569 -17.5632 30.9276 -18.4109 29.8393 "
        "-18.8967 28.7513 -19.7444 27.6627 -20.5261 26.5108 -21.0079 25.177 "
        "-21.9253 23.9139 -22.828 22.821 -23.1992 22.1008 -24.0365 20.9427 "
        "-25.369 20.0365 -26.4588 19.554 -28.0325 19.0722 -29.3022 18.7057 "
        "-30.3902 18.5203 -31.7832 18.1658 -33.8402 18.1024 -36.6239 18.0346 "
        "-38.194 18.168 -39.4066 18.7066 -40.9821 18.8861 -41.8828 19.8016 "
        "-42.7958 20.5213 -43.638 21.677 -44.9124 22.5793 -46.0601 23.4883 "
        "-47.0895 24.7583 -48.4158 26.2728 -49.0205 27.5445 -49.9332 28.9932 "
        "-50.6053 30.1997 -51.3294 31.5324 -52.0559 33.3476 -52.6599 35.2244 "
        "-52.832 37.524 -52.9632 40.3071 -52.8963 41.6371 -52.2953 43.3327 "
        "-51.4457 45.2071 -50.5427 46.9044 -49.6342 48.054 -48.6021 49.0178 "
        "-46.6024 50.4055 -45.6969 51.0725 -44.6091 51.6192 -43.4626 52.1116 "
        "-42.2511 52.5329 -40.6794 52.8364 -39.4073 53.3791 -38.2573 53.6273 "
        "-36.2602 53.6861 -34.1431 53.6154 -32.5699 53.3747 -31.2987 52.954 "
        "-29.9644 52.6575 -29.057 52.2321 -27.3612 51.262 -26.945 50.7107 "
        "-26.152 50.1131 -25.4303 49.4429 -24.4624 48.2931 -23.8513 47.6927 "
        "-23.0672 46.6625 -22.3478 45.7507 -21.3816 43.5738 -21.2542 41.759 "
        "-21.3195 39.5823 -21.8588 38.2494 -22.1056 36.7382 -22.7075 34.8615 "
        "-23.1298 33.7113 -24.0362 32.0162 -24.9486 30.7492 -25.8552 29.5991 "
        "-26.7565 28.6256 -27.3631 27.8976 -28.3979 27.3643 -29.9682 26.4519 "
        "-31.1229 25.7988 -32.1465 25.3635 -33.5363 24.8757 -34.4452 24.3966 "
        "-36.441 24.518 -37.1709 24.7485 -38.2592 25.1163 -39.1628 25.6073 "
        "-40.0135 26.1461 -41.4581 27.122 -42.3713 28.1456 -43.0962 29.1148 "
        "-43.8784 29.904 -44.5442 30.8725 -45.2831 31.7732 -45.7551 33.1688 "
        "-46.1888 34.4966 -46.6083 35.8291 -46.9069 37.0402 -47.1475 38.0682 "
        "-47.1492 38.9754 -46.8545 40.43 -46.2991 41.9972 -45.6932 42.7837 "
        "-45.1551 43.6355 -44.1848 44.359 -43.2796 45.2698 -41.464 46.2369 "
        "-40.1921 46.8347 -38.257 47.163 -36.26 47.1469 -33.9001 46.8433 "
        "-31.8414 46.1781 -29.9634 44.4849 -29.0066 43.1475 -28.0868 41.215 "
        "-27.9107 39.6413 -27.9061 37.8249 -28.397 36.0121 -29.1777 34.3144 "
        "-29.7883 33.4717 -30.5184 32.749 -31.2997 31.8368 -32.0245 30.9875 "
        "-33.6592 30.4384 -35.1738 30.5576 -36.7427 31.0522 -37.957 31.8935 "
        "-39.1621 32.8065 -40.1912 34.0156 -40.9207 35.2241 -41.2192 36.072 "
        "-41.4701 37.524 -41.4003 38.9729 -40.6814 39.7076 -39.7649 40.3627 "
        "-39.1049 41.0342 -38.3799 41.4658 -36.8688 41.7504 -35.4709 41.218 "
        "-34.6251 40.551 -33.6002 39.7018 -33.5932 38.3712 -33.5286 37.5195 "
        "-33.9003 36.7325 -34.6258 36.6815 -35.4775 36.8556 -36.0816 37.7042 "
        "-36.566 38.9716 -37.0469 39.3366 -38.2579 39.3398 -39.0989 38.6115 "
        "-39.4593 37.1642 -38.7312 35.5404 -37.3482 34.5009 -35.9581 33.8375 "
        "-34.4416 33.8857 -33.2942 34.4366 -32.325 35.5254 -31.5431 36.7376 "
        "-31.1787 38.6129 -30.9989 40.4876 -31.4768 41.8841 -32.4452 42.7909 "
        "-33.2942 43.6376 -34.1431 44.0562 -34.99 44.2371 -36.2615 44.3447 "
        "-37.1073 44.4205 -38.1362 44.3005 -39.346 43.7554 -40.5034 43.2234 "
        "-41.1665 42.7922 -41.5827 42.3019 -42.3112 41.3961 -43.0321 40.1236 "
        "-43.5829 38.6736 -43.8213 37.4632 -43.8216 36.0719 -43.5748 34.8022 "
        "-43.3994 33.8332 -42.8629 32.7981 -42.2488 32.0799 -41.4058 31.2287 "
        "-40.5573 30.383 -39.768 29.7219 -38.8039 29.229 -37.3487 28.6323 "
        "-35.5948 28.1452 -34.0216 28.1429 -31.6015 28.2664 -30.4013 29.0031 "
        "-29.2452 30.084 -27.9715 31.8968 -27.128 33.1694 -26.0975 35.044 "
        "-25.3043 36.4342 -25.1326 38.9761 -25.0111 40.7297 -25.2416 42.1865 "
        "-25.8542 43.2711 -26.754 44.3055 -27.9737 45.931 -29.0659 46.8355 "
        "-30.2076 47.9336 -31.4242 48.8215 -33.1146 49.2578 -35.5345 49.5011 "
        "-37.5918 49.6921 -40.1321 49.5581 -42.4918 49.3812 -44.4291 48.6577 "
        "-45.9436 47.8118 -46.7291 46.9636 -47.3306 46.2349 -48.1181 45.1466 "
        "-48.9089 43.878 -49.3322 42.9098 -49.8164 41.8209 -50.181 40.9729 "
        "-50.3659 39.1555 -50.0582 37.8255 -49.699 36.3711 -49.2196 35.278 "
        "-48.9029 34.8016 -48.609 33.4086 -48.4211 32.0788 -47.8754 31.2319 "
        "-47.4493 30.0238 -46.6062 28.6298 -45.6973 27.2408 -44.4897 26.4506 "
        "-43.095 25.3054 -41.1545 24.042 -40.0092 23.3096 -38.5014 22.4527 "
        "-36.8047 21.7331 -34.6278 21.1918 -32.5695 21.6711 -30.4434 21.9618 "
        "-28.8776 23.0603 -27.485 23.6641 -26.1587 24.3977 -25.371 25.0008 "
        "-24.4095 25.9118 -23.4891 27.4132 -22.411 28.2697 -21.7333 29.5336 "
        "-21.1339 30.5654 -20.4702 31.5346 -19.8096 32.6257 -19.3826 33.5921 "
        "-18.6606 35.3473 -18.1737 37.2217 -17.9887 38.7339 -17.7493 40.6094 "
        "-17.7486 42.6662 -18.0415 44.7866 -18.6544 46.599 -19.6201 48.2947 "
        "-20.406 49.324 -21.4951 50.536 -22.7053 51.2609 -24.0393 52.2857 "
        "-25.5549 53.4316 -26.4563 54.167 -27.853 54.8844 -28.7566 55.4969 "
        "-29.7855 55.8596 -31.1175 56.2804 -32.5694 56.6505 -34.99 56.7094 "
        "-36.5625 56.6397 -37.5916 56.5228 -38.8015 56.4015 -39.7102 56.4041 "
        "-40.5563 56.0997 -41.6459 56.0422 -43.0397 55.8022 -44.0647 55.309 "
        "-45.3377 55.0141 -46.6648 54.5217 -48.1815 53.6211 -49.142 53.0657 "
        "-50.1875 52.2988 -50.9639 51.5613 -51.5045 50.65 -52.2368 49.9898 "
        "-52.9014 49.0211 -53.5683 48.0527 -53.7482 47.5079 -54.2966 46.4202 "
        "-54.6512 45.5094 -55.074 44.2993 -55.2525 43.6336 -55.4425 42.486 "
        "-55.8588 41.4565 -55.8658 40.852 -56.1035 39.9431 -56.5227 38.794 "
        "-56.5962 37.5853 -56.7705 36.5557 -56.6505 35.5271 -56.4755 34.4346 "
        "-56.1646 33.8334 -55.9183 32.8658 -55.7424 31.7154 -55.4415 31.1094 "
        "-54.8927 30.2039 -54.5315 29.2956 -54.108 28.207 -53.3248 27.3578 "
        "-52.6554 26.2106 -52.1134 25.4829 -51.5707 24.3312 -50.7863 23.4829 "
        "-50.1165 22.6393 -49.4568 21.7283 -48.8518 20.6982 -48.0615 19.734 "
        "-47.447 18.6513 -46.7768 17.7536 -46.0021 17.1333 -45.034 16.1042 "
        "-43.6419 15.2582 -42.5546 14.7656 -41.1007 14.4703 -39.7695 14.4733 "
        "-38.3774 14.3582 -36.2604 14.1698 -34.4453 14.2351 -32.3863 14.2147 "
        "-30.3308 14.4639 -28.3327 14.5277 -26.2799 15.3265 -24.7103 15.8756 "
        "-22.5273 16.9542 -21.8577 17.4345 -21.193 18.402 -20.4708 18.9505 "
        "-19.8028 19.7956 -19.0759 20.4608 -18.4072 21.7294 -17.8693 22.6408 "
        "-17.3896 23.6121 -16.5354 24.8781 -15.9304 26.2696 -15.3892 27.2999 "
        "-14.8984 28.2642 -14.2938 29.2935 -13.7572 30.5072 -13.3296 31.4734 "
        "-12.903 32.44 -12.3583 33.5899 -11.8816 34.9237 -11.2159 36.6178 "
        "-10.7238 37.7645 -10.3051 39.2178 -10.2431 40.7301 -9.70324 41.8805 "
        "-9.51718 43.3322 -9.81727 44.543 -10.3073 46.114 -10.8437 47.5095 "
        "-11.6893 49.0823 -12.303 50.7118 -13.2138 51.7975 -14.1748 52.8328 "
        "-15.0818 53.8012 -15.937 54.702 -16.7208 55.6716 -17.5007 56.4657 "
        "-18.474 57.246 -19.1371 57.8541 -19.9266 58.332 -21.1281 58.8874 "
        "-21.9811 59.79 -23.3729 60.455 -24.401 61.182 -25.3064 61.6102 -26.2157 "
        "62.0304 -27.367 62.448 -28.5165 62.6908 -29.7262 62.9936 -30.8752 "
        "63.1205 -32.3276 63.2432 -33.5982 63.2375 -35.2318 63.2364 -36.1998 "
        "63.2364 -37.4101 63.1172 -38.9226 62.8153 -40.1331 62.8163 -41.3417 "
        "62.5081 -42.4313 62.2073 -44.0055 61.848 -44.9719 61.5401 -46.0023 "
        "61.2438 -47.3328 60.8182 -48.2424 60.3981 -48.9667 59.9716 -50.1782 "
        "59.4301 -51.2046 58.9417 -52.0531 58.3994 -53.0224 57.7964 -53.5086 "
        "57.4966 -54.2878 56.9412 -54.9559 56.5217 -55.5601 55.976 -56.4747 "
        "55.3178 -56.9505 54.825 -57.4913 54.4005 -57.916 53.734 -58.3491 "
        "53.3176 -58.5851 52.8304 -59.1298 51.9828 -59.6144 51.6208 -59.9141 "
        "51.1348 -60.3397 50.5312 -60.7664 49.9277 -60.9439 49.3808 -61.5548 "
        "48.5984 -62.0352 47.87 -62.2144 47.3238 -62.7008 46.6599 -63.0658 "
        "46.056 -63.5524 45.2099 -63.9092 44.4204 -64.273 43.6948 -64.5709 "
        "42.9668 -64.8841 42.2457 -65.1832 41.6387 -65.4683 40.7245 -65.7827 "
        "39.9427 -66.0874 39.0965 -66.3992 37.7665 -66.4534 37.1609 -66.6288 "
        "35.8889 -66.8712 34.9811 -67.0559 34.438 -67.0557 33.5304 -67.0602 "
        "32.6227 -66.9928 31.7156 -66.9533 30.8683 -66.9383 29.8397 -66.9263 "
        "29.0539 -66.8729 28.2668 -66.8236 27.3578 -66.6969 26.5109 -66.3943 "
        "25.6642 -66.0922 24.5746 -65.6619 23.4878 -65.37 22.4538 -64.8782 "
        "21.6719 -64.4534 20.7046 -64.0386 19.9738 -63.3093 18.6442 -62.8256 "
        "17.8577 -62.1598 16.7081 -61.4843 15.6252 -61.0649 14.9572 -60.6401 "
        "14.3522 -60.2194 13.4436 -59.7328 12.78 -59.1336 11.9895 -58.5793 "
        "11.0268 -58.2201 10.3614 -57.5542 9.81519 -57.1374 9.14484 -56.6504 "
        "8.54221 -55.9257 7.8149 -55.3159 7.15355 -54.5954 6.30196 -53.922 "
        "5.40095 -53.1381 4.49142 -52.419 3.75688 -51.7501 3.27756 -50.8383 "
        "2.61624 -49.9314 1.64865 -48.9657 0.919663 -48.2405 0.494643 -47.2714 "
        "0.0127581 -46.5421 -0.344675 -45.4614 -0.963183 -44.4973 -1.63675 "
        "-43.3404 -2.29018 -42.3102 -2.77096 -41.4643 -3.37826 -40.1971 -3.92937 "
        "-39.2261 -4.40723 -38.3775 -4.88676 -37.2897 -5.07389 -35.8364 -5.49333 "
        "-34.8671 -5.91258 -33.297 -6.4053 -31.8465 -7.01358 -30.6357 -7.55535 "
        "-29.3056 -8.34424 -28.2725 -8.88058 -27.245 -9.54744 -26.2147 -9.90444 "
        "-25.186 -10.213 -24.7734 -11.3646 -24.6349 -12.2108 -24.7152 -13.5991 "
        "-25.0115 -14.7491 -25.5963 -15.908 -26.0307 -16.8694 -26.0797 -18.0811 "
        "-26.2151 -19.1685 -26.2802 -20.3782 -26.4075 -21.4695 -26.2138 -22.3134 "
        "-25.7295 -23.4624 -25.3056 -24.2502 -25.131 -25.4615 -24.8259 -26.8526 "
        "-24.5853 -27.881 -24.5183 -28.7883 -24.455 -29.9378 -24.3993 -31.4508 "
        "-24.4606 -33.0846 -24.703 -34.1741 -25.8635 -35.8598 -26.462 -36.652 "
        "-26.9976 -37.5042 -27.5463 -38.4706 -28.1566 -39.071 -28.824 -39.857 "
        "-29.3058 -40.5843 -29.8478 -41.3712";

    auto shape = Polygon::Circle(100, 0, 0, 6);
    PolygonMeshBuilder("ground", shape, scene).addHole(Polygon::Parse(monkeyData)).build();
  }

}; // end of struct PolygonMeshMonkeyScene

BABYLON_REGISTER_SAMPLE("Meshes", PolygonMeshMonkeyScene)

} // end of namespace Samples
} // end of namespace BABYLON
