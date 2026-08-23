/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *   by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lmmp/impl/log2_exp2.h"
#include "../../../include/lmmp/impl/longlong.h"
#include <stdbool.h>


/*
    64bit 版本的 log2 耗时大约 27 ns
    64bit 版本的 exp2 耗时大约 15 ns
    128bit 版本的 log2 耗时大约 110 ns
    128bit 版本的 exp2 耗时大约 75 ns

    计算原理为切比雪夫最佳估计，为了保证至多只有1的误差，
    64bit版本实际运算为128bit，而128bit版本实际运算为192bit.
    但是用以估计的多项式的最大误差仅仅略低于2^-64或者2^-128，这是为了
    减少多项式项数以减少计算量。需要注意的是，为了保证不会导致溢出，
    log2的系数展开式都除以了2（这是因为部分系数可能超过1，转化成定
    点数仅表示小数时会导致整数部分丢失），也就是说实际计算的是log2(1+x)/2，
    通过保证计算精度大于64bit，再通过左移1位进行恢复。

    log2版本的系数由于有正有负，所以进行了有符号处理，
    而exp2版本的系数由于都是正数，直接无符号计算。

    由于log2(1+x)在(0,1)区间的多项式拟合效果远差于exp2，所以在128bit版本中，
    我们使用了分段估计的方法，分成了(0,0.25),[0.25,0.5),[0.5,0.75),[0.75,1)四个区间，
    每个区间使用不同的多项式进行估计，这样可以减少计算量。
*/
#if 0
#define EXP2_COEFFS_SIZE 24
static const uint64_t exp2_coeffs[][3] = {{0xffca0b964298076cULL, 0xffffffffffffffffULL, 0xffffffffffffffffULL},
                                          {0x33bda4e46bbc7806ULL, 0xc9e3b39803f2f6b0ULL, 0xb17217f7d1cf79abULL},
                                          {0x85f40437ee8f0800ULL, 0xde2d60dd92e6bedfULL, 0x3d7f7bff058b1d50ULL},
                                          {0x2c66f99092afcfefULL, 0x99d3b15d995ecd1aULL, 0x0e35846b82505fc5ULL},
                                          {0xca8881c9d6af26a4ULL, 0x39977c16a7d4c8ceULL, 0x0276556df749cee5ULL},
                                          {0x917458ca2f264f65ULL, 0x41c5fda69527fbeaULL, 0x005761ff9e299cc4ULL},
                                          {0x3d5310cc16f7288bULL, 0xb7a58544b576821fULL, 0x000a184897c363c3ULL},
                                          {0x77f4544d7045520cULL, 0x34358a8f08ed0eaaULL, 0x0000ffe5fe2c4586ULL},
                                          {0xfe4fe846d143293eULL, 0x23fd8ff8bb26dd48ULL, 0x0000162c0223a5c8ULL},
                                          {0x82ee72c9e032ee4cULL, 0x7c3da4cccdebc263ULL, 0x000001b5253d395eULL},
                                          {0x9b9df4a1c35db3fcULL, 0x8ec9f6392bf93c4fULL, 0x0000001e4cf5158bULL},
                                          {0xb55e391e9b0ab1b7ULL, 0x1bb24f37e0ad183fULL, 0x00000001e8cac735ULL},
                                          {0x0a29dfc4c8e6a7d6ULL, 0xfc297b7d49d4135aULL, 0x000000001c3bd650ULL},
                                          {0x0e048584b5f3d960ULL, 0x166d2b2b6b097705ULL, 0x0000000001816193ULL},
                                          {0x9c047e7d94358576ULL, 0x4d583d67e3a6aa44ULL, 0x0000000000131496ULL},
                                          {0xdefebfed06d08e44ULL, 0x421de93a99954648ULL, 0x000000000000e1b7ULL},
                                          {0x52a436ec2b5db367ULL, 0x44d6ab6b7b988e29ULL, 0x00000000000009c7ULL},
                                          {0xbbad553f63386545ULL, 0x111375bdff904874ULL, 0x0000000000000066ULL},
                                          {0x71ea9f8f0b267dd1ULL, 0xee2e420000724148ULL, 0x0000000000000003ULL},
                                          {0x9de7f0e17da29b1dULL, 0x24b5681147f4fd49ULL, 0x0000000000000000ULL},
                                          {0x4b1c7f76e0cb2c00ULL, 0x014572b9e9826ee9ULL, 0x0000000000000000ULL},
                                          {0x8fea0333c968e6b1ULL, 0x000ad737690d1c6eULL, 0x0000000000000000ULL},
                                          {0x79f4f3c8f4281f5fULL, 0x0000503ad6683802ULL, 0x0000000000000000ULL},
                                          {0x3015d9c1fc532f45ULL, 0x000003b3200ce020ULL, 0x0000000000000000ULL}};

static const uint64_t log2_coeffs_1[][3] = {{0x0053e37eeedc0e03ULL, 0x0000000000000000ULL, 0x0000000000000000ULL},
                                            {0x6e9689ba68249277ULL, 0xbe87fed0691d3e7eULL, 0xb8aa3b295c17f0bbULL},
                                            {0x4ef0d80aa48780daULL, 0xdf43ff68348e675bULL, 0x5c551d94ae0bf85dULL},
                                            {0xb670470ff80d57baULL, 0x3f82aa4577e824e6ULL, 0x3d8e13b87407fae9ULL},
                                            {0x3eee5d38fb48bdd6ULL, 0xefa1ffb39355c392ULL, 0x2e2a8eca5705fc2eULL},
                                            {0x0f1e52e491454efeULL, 0x594e65cb06c937d9ULL, 0x24eed8a1df37fcf2ULL},
                                            {0xf5f28693b4bc365bULL, 0x9fc1283f02470a74ULL, 0x1ec709dc3a03fd74ULL},
                                            {0x88044f9b7b99ab07ULL, 0x644d4acab5bfe4baULL, 0x1a61762a7aded93fULL},
                                            {0x52dcf3fd623bb310ULL, 0x73ec23907ec21677ULL, 0x171547652b82fe17ULL},
                                            {0x86ee9d918454c620ULL, 0xa62c2c4ed4a8a044ULL, 0x1484b13d7c02a8f7ULL},
                                            {0x4928052ed5acf299ULL, 0x90f606b8fa609444ULL, 0x12776c50ef9bfe5aULL},
                                            {0x0862cf9d15220723ULL, 0x03e9e47812c98507ULL, 0x10c9a8499402294fULL},
                                            {0x2db35871a8faae45ULL, 0x79ffa155959a8760ULL, 0x0f6384ee1d01994aULL},
                                            {0xc9c203142a5fd00aULL, 0xc522eff23a0a4d9cULL, 0x0e347ab469830951ULL},
                                            {0x8c418ed5dfa6a642ULL, 0x60b3408cd6210b77ULL, 0x0d30bb153cd171e7ULL},
                                            {0x3a528ea45f011dc2ULL, 0x7f989cac87350b63ULL, 0x0c4f9d8b40e8fcf6ULL},
                                            {0xc7787f3eba9b7e6dULL, 0x23b299c84d57418bULL, 0x0b8aa3b219e2f3caULL},
                                            {0xe9f09a984ee0fcc6ULL, 0xfc06f6a34e76409bULL, 0x0adcd6485a51183fULL},
                                            {0x9c013212dc14fa0aULL, 0x3ac3aeb2dba45320ULL, 0x0a42586bc0c837c9ULL},
                                            {0x305d155af52571cdULL, 0xb6c6379af9b9c75bULL, 0x09b81c71926a118aULL},
                                            {0xaa303aaba3c7d0b4ULL, 0x7a409595413b27f8ULL, 0x093baae79f96b7b6ULL},
                                            {0x3c5ba98762dbb059ULL, 0x6116ad84b6ba7ae4ULL, 0x08cae454858a594eULL},
                                            {0xa6fdbf1ed25a325cULL, 0x6fb30123eabbe68cULL, 0x08637e56410b5f99ULL},
                                            {0x6861f7851d433fceULL, 0xbae222e8cb58c178ULL, 0x08019836f4ee374aULL},
                                            {0xee5fb1c33fb5b6adULL, 0x9987c9c3201ad273ULL, 0x079c4ab07eab2dd3ULL},
                                            {0x4abd4b8fc52cf610ULL, 0x1a21966f2bef275aULL, 0x071fd43ab3f489deULL},
                                            {0xa181b8a8d6b16d04ULL, 0xf70cf02db2d53a48ULL, 0x0669a9aaef3cde95ULL},
                                            {0x3cb546ced26898b5ULL, 0xfed46a22a76ae0b0ULL, 0x0552ea0690c5f831ULL},
                                            {0x650f422da5842eaaULL, 0xa79de6eceb25ffebULL, 0x03d438de0e35125eULL},
                                            {0xe666a424f2412c60ULL, 0x6d3d6b19feeb47a8ULL, 0x022e6df7514dd774ULL},
                                            {0x9cd33fb2ba2f17a3ULL, 0x1bcb7d0743dae1a0ULL, 0x00dae199271128a3ULL},
                                            {0x005011d0ddd86c0dULL, 0x3cd83f3a0af29abcULL, 0x002b73c6cf827132ULL}};

static const bool log2_coeffs_bool_1[] = {true,  true,  false, true,  false, true,  false, true,  false, true,  false,
                                          true,  false, true,  false, true,  false, true,  false, true,  false, true,
                                          false, true,  false, true,  false, true,  false, true,  false, true};

static const uint64_t log2_coeffs_2[][3] = {{0xa57fd42cd46583e2ULL, 0x356fe20b8ef53ca7ULL, 0x0000000000000000ULL},
                                            {0x8e7ead5c2021e7e9ULL, 0xa6acab23117f9b8eULL, 0xb8aa3b295c17f0a8ULL},
                                            {0xe858816fd9215646ULL, 0xe4a709f34129ea19ULL, 0x5c551d94ae0bf510ULL},
                                            {0xd47150fd7781a288ULL, 0x57d156338dd97218ULL, 0x3d8e13b874079cc4ULL},
                                            {0x0bf7c0676f5ce7eeULL, 0xe17bbdbc4b887646ULL, 0x2e2a8eca56fe632eULL},
                                            {0xbec9564a9bb7e72fULL, 0x6978877c40639611ULL, 0x24eed8a1debed736ULL},
                                            {0xd7459e4838a49cd9ULL, 0x5759ebb5ab9b44ccULL, 0x1ec709dc33f4ae3cULL},
                                            {0x3d1fe612de917be5ULL, 0x63f146495b801bfbULL, 0x1a61762a3ad9eb46ULL},
                                            {0x6f9383dbf0a41795ULL, 0x8930769849d3c115ULL, 0x17154762f29c3395ULL},
                                            {0x75b26400fe893a89ULL, 0xa9a170c4248904dfULL, 0x1484b12ca1dc718fULL},
                                            {0xb6df3ce4c62b22b8ULL, 0x209f124ebf943679ULL, 0x12776be2accb2184ULL},
                                            {0x9773271e0478b1e1ULL, 0xc3bdd48193d60ea3ULL, 0x10c9a5d5cca40e55ULL},
                                            {0x2e25832795801ee5ULL, 0xa00530afa5bcad68ULL, 0x0f6378b2d9ff05d9ULL},
                                            {0x43e9fa3cdcb9c7dfULL, 0xff5fc357852dfff2ULL, 0x0e3444f9a01b492bULL},
                                            {0xd1e7259b2434529fULL, 0xf721115b9c55d36dULL, 0x0d2fea5b3a7720f6ULL},
                                            {0xb2d6e66b3337da21ULL, 0x081e62e31eb10bb1ULL, 0x0c4cce1c6e47ea29ULL},
                                            {0x7da8d12f4da490d5ULL, 0x52515efd345707cdULL, 0x0b8205ed1eb997dfULL},
                                            {0x478f940dd8cf9775ULL, 0xb0ca8426a47fbf03ULL, 0x0ac54a8d47c6863fULL},
                                            {0xcfa9312565c2843fULL, 0x0f759d1f08c6799eULL, 0x0a08e77ddd1fbb2dULL},
                                            {0xf48548dbbba05ec6ULL, 0xdd574390deeda42fULL, 0x093ad80d0dd3ecbaULL},
                                            {0x017cc0ac677ea013ULL, 0x7e46d10e050bc02dULL, 0x08472381fda36eabULL},
                                            {0x871584f285057ee9ULL, 0x97291de5663f220bULL, 0x071efb1899675facULL},
                                            {0x6f34fba787f124faULL, 0x479af528189583f0ULL, 0x05c2f71e630913ecULL},
                                            {0x4bd4738c010d7fc0ULL, 0x824bf28002150390ULL, 0x044a4d4672ca372aULL},
                                            {0x0bdee63c9db584c1ULL, 0x33573128f2ffd048ULL, 0x02df66962d8231e0ULL},
                                            {0xf6c1a590698a9261ULL, 0xa08ed6ba9e5ae35aULL, 0x01afe1f1bff10ce9ULL},
                                            {0x0ea2e2942145e6a8ULL, 0x54d97ba93893517eULL, 0x00d851eeb1d21558ULL},
                                            {0xd43a93493dcce900ULL, 0x34097ec6fe7a23dfULL, 0x005955f11886ad76ULL},
                                            {0x1bc3be0d4018566dULL, 0xb27b75038f03ad91ULL, 0x001d145afcb6e7b6ULL},
                                            {0x8a832b5d04244d02ULL, 0xf115fbec8af54366ULL, 0x0006f9122e39c360ULL},
                                            {0x97492d8a5e1c44f5ULL, 0x68316fbae37b7f21ULL, 0x000117aaabce7103ULL},
                                            {0xbbc9f19c75261f15ULL, 0xd97d9d7070bc73f3ULL, 0x000015700247c7e5ULL}};

static const bool log2_coeffs_bool_2[] = {true,  true,  false, true,  false, true,  false, true,  false, true,  false,
                                          true,  false, true,  false, true,  false, true,  false, true,  false, true,
                                          false, true,  false, true,  false, true,  false, true,  false, true};

static const uint64_t log2_coeffs_3[][3] = {{0x7157382233405391ULL, 0x8963b7a7e51286c3ULL, 0x000000000007fdd7ULL},
                                            {0xc07b41ec3a38f6e7ULL, 0x3906b0d593b435acULL, 0xb8aa3b295a95e6cdULL},
                                            {0xb7baecd1b1e725dfULL, 0x48749c56d0e05e4fULL, 0x5c551d948ad7c93aULL},
                                            {0x7df0527b8d3d5a10ULL, 0x617e0037ec390878ULL, 0x3d8e13b662fec24cULL},
                                            {0x23d187153463476fULL, 0x4339f0444bfaf385ULL, 0x2e2a8eb3dfb9e7f2ULL},
                                            {0x823643d619b14effULL, 0xdd20749ee30e0891ULL, 0x24eed7e59eeb9256ULL},
                                            {0xd163439f50cd49eeULL, 0xb8ca0861c1c38b30ULL, 0x1ec704eb2cc83965ULL},
                                            {0xeab485a64112d275ULL, 0xce40d28c75106df6ULL, 0x1a615ace561169daULL},
                                            {0x1fd23a118ad5c4efULL, 0x8ae254c3bfec2c33ULL, 0x1714c824a8be4d9dULL},
                                            {0x60052bbe9a261179ULL, 0x337327b6c5086c91ULL, 0x1482b8c880c8a10aULL},
                                            {0xed35f3e3c0f17b9dULL, 0xfec4d48544c197c3ULL, 0x1270b09b22de0c19ULL},
                                            {0xe8f5028d601960e1ULL, 0xcbdfe2ef13da3e1eULL, 0x10b5a6273bb47bfbULL},
                                            {0x012326e5e454d8ecULL, 0xf913bec6eecfd9d9ULL, 0x0f2f6f7d2615fb93ULL},
                                            {0x9ce910375c1c33daULL, 0x4a0a16e12338a9abULL, 0x0dbcfa6c1bdc5803ULL},
                                            {0xbaab6d7a2f1e659dULL, 0x251d045f0d811e69ULL, 0x0c3dbc39121f58b1ULL},
                                            {0x86d5ea8856d209cbULL, 0xb94202a84acba33fULL, 0x0a976b697968e92fULL},
                                            {0x7ff0a25ee979d8c4ULL, 0xde4ae3a50f8226cdULL, 0x08c05f9115f5caaeULL},
                                            {0xe603325bdcb7afa0ULL, 0x63314235e5535fe9ULL, 0x06c8608576f422a1ULL},
                                            {0x32855705b8f8f359ULL, 0x23f6ceb2d4a25bbfULL, 0x04d7b898bae7bb95ULL},
                                            {0x1d00a3792c5e0c89ULL, 0x6a7b7216d627d713ULL, 0x032115a194fd21feULL},
                                            {0x3a49e697e4321db0ULL, 0xbb036bb4e9067a6eULL, 0x01cc5fe090781339ULL},
                                            {0x5169a5d51cbeb0e5ULL, 0xd602a5a9ee06058dULL, 0x00e7188a93e7a34bULL},
                                            {0x045f99570559a1a5ULL, 0x2497a5d804f2876bULL, 0x00635695b7da83d2ULL},
                                            {0xa9268e9189703c08ULL, 0x19296996c2c5cf45ULL, 0x0023b7f31dd76057ULL},
                                            {0x035e6ad91be15771ULL, 0x30fb752d65046f04ULL, 0x000a6cb0398ad366ULL},
                                            {0xc91789f25fafd692ULL, 0xb8bc3e51fe1fee5eULL, 0x00025e28b6bb39ddULL},
                                            {0x16a58a08648f1645ULL, 0xa5d8a4b6b67100c1ULL, 0x00006461dce69ed5ULL},
                                            {0xc44adf513e3db58bULL, 0xb8f95283e0a87b10ULL, 0x00000ac50718e454ULL},
                                            {0x289246c50a5c7455ULL, 0x6a2390cac25d1e96ULL, 0x0000008fb5133fb6ULL}};

static const bool log2_coeffs_bool_3[] = {true,  true, false, true, false, true, false, true, false, true,
                                          false, true, false, true, false, true, false, true, false, true,
                                          false, true, false, true, false, true, false, true, false};

static const uint64_t log2_coeffs_4[][3] = {{0x2ffb9355c9b02da7ULL, 0xd517158f1f57e165ULL, 0x000000000b26e43dULL},
                                            {0xb0d8a65353ce9560ULL, 0x98a1f9ba8a0dbb7dULL, 0xb8aa3b27dc25fb0cULL},
                                            {0xe11aef451b13eadbULL, 0x6327a9efd5f0e7f5ULL, 0x5c551d7bb18d6343ULL},
                                            {0x1e8308ca45a27785ULL, 0xf9829c7bf931cc97ULL, 0x3d8e12ac1bc48bf4ULL},
                                            {0x1407edbddecee382ULL, 0xe634c11636b19f3fULL, 0x2e2a86a21d72c68cULL},
                                            {0xaf06796142b1850aULL, 0x92b7ad3b826cc637ULL, 0x24eea79cf13731fbULL},
                                            {0xc852fa6c4fda4a79ULL, 0xe4fa7bb254ae88f9ULL, 0x1ec61d1a72a74f28ULL},
                                            {0xfd7654cf8fdca047ULL, 0xe47275e3893cab8dULL, 0x1a5dc5996d42ba35ULL},
                                            {0xe886380f53c574baULL, 0x605439c39cd0a007ULL, 0x1708dffaed5dfdd3ULL},
                                            {0xb615450a7c78b361ULL, 0x53eeeceb1118f59bULL, 0x1461049f70ca276aULL},
                                            {0x227ba4a18351bc66ULL, 0x0f9f1cfeacc1f4a0ULL, 0x121e94eff999c2d3ULL},
                                            {0xc4822d9225e094a9ULL, 0xa79b079a47ddc670ULL, 0x1008275ed6a05eacULL},
                                            {0x878cc26815a042b9ULL, 0x94117e8965fc1878ULL, 0x0defa9d9ec9faf23ULL},
                                            {0xb11703e9215bde7eULL, 0x4d9fa83c1b5f908cULL, 0x0bb8dde62ab01c89ULL},
                                            {0xd55ff7949f47706dULL, 0xb79a65001b72518dULL, 0x09628ceec13a86e7ULL},
                                            {0x5c61477e458aa647ULL, 0xc1229b479fdd0a04ULL, 0x07094c4233dd0c69ULL},
                                            {0xd3d6d2501b1c3799ULL, 0xa34944fd0b6f34e9ULL, 0x04dd864ce0e8d1e3ULL},
                                            {0xe491d8aeea1b455eULL, 0xdc90c0b677d36f75ULL, 0x030f38c2c1f6403eULL},
                                            {0x8cdaaeb07ab1277dULL, 0x8807ff5ed94d5f38ULL, 0x01ba1a229b454744ULL},
                                            {0xd47d54251ab3f8c5ULL, 0xc01ab228aa5c99b9ULL, 0x00dd3271f406c177ULL},
                                            {0xab38b30542d7a3edULL, 0x3ff1d1dfcd1d0d8cULL, 0x0060d42d8c9f8417ULL},
                                            {0x6201a1571d58f282ULL, 0xabd5a0ce256d231dULL, 0x00248e962fc41fa7ULL},
                                            {0xd89195c384a009c9ULL, 0x854ef3d68b6adba1ULL, 0x000bb43dab6c17dcULL},
                                            {0x3e136ed464d490bbULL, 0x3f964f005856092aULL, 0x00031c65965ca2fcULL},
                                            {0xdde7cef0c826a0a5ULL, 0xefbca2e72bbdb691ULL, 0x0000aad2a5c8663fULL},
                                            {0x07cba043ef509ed7ULL, 0xe47c73acad9d1761ULL, 0x00001c633e618000ULL},
                                            {0xb2e07d352d1caea3ULL, 0x8b9779727e7c6482ULL, 0x0000036d6c34e399ULL},
                                            {0xa9fc82622911571fULL, 0x0c091200972a754dULL, 0x00000044714090a6ULL},
                                            {0xa427d253633082f0ULL, 0x0b72a7d8935e4cc0ULL, 0x0000000296926cdcULL}};

static const bool log2_coeffs_bool_4[] = {true,  true, false, true, false, true, false, true, false, true,
                                          false, true, false, true, false, true, false, true, false, true,
                                          false, true, false, true, false, true, false, true, false};

#define LOG2_COEFFS_SIZE_1 32
#define LOG2_COEFFS_SIZE_2 32
#define LOG2_COEFFS_SIZE_3 29
#define LOG2_COEFFS_SIZE_4 29

static inline void umul192x128_tohi192(uint64_t dst[3], const uint64_t i192[3], const uint64_t i128[2]) {
    uint64_t a0 = i192[0], a1 = i192[1], a2 = i192[2];
    uint64_t b0 = i128[0], b1 = i128[1];

    uint64_t p00_l, p00_h, p01_l, p01_h, p10_l, p10_h;
    uint64_t p11_l, p11_h, p20_l, p20_h, p21_l, p21_h;

    _umul64to128_(a0, b0, &p00_l, &p00_h);
    _umul64to128_(a0, b1, &p01_l, &p01_h);
    _umul64to128_(a1, b0, &p10_l, &p10_h);
    _umul64to128_(a1, b1, &p11_l, &p11_h);
    _umul64to128_(a2, b0, &p20_l, &p20_h);
    _umul64to128_(a2, b1, &p21_l, &p21_h);
    /*
    | res0 | res1 | res2 | res3 | res4 | res5 |
           | p00l | p00h |
                  | p01l | p01h |
                  | p10l | p10h |
                         | p11l | p11h |
                         | p20l | p20h |
                                | p21l | p21h |
    */

    uint64_t carry = 0;
    p00_h += p01_l;
    carry += p00_h < p01_l ? 1 : 0;
    p00_h += p10_l;
    carry += p00_h < p10_l ? 1 : 0;

    dst[0] = p01_h + carry;
    carry = dst[0] < carry ? 1 : 0;
    dst[0] += p10_h;
    carry += dst[0] < p10_h ? 1 : 0;
    dst[0] += p11_l;
    carry += dst[0] < p11_l ? 1 : 0;
    dst[0] += p20_l;
    carry += dst[0] < p20_l ? 1 : 0;

    dst[1] = p11_h + carry;
    carry = dst[1] < carry ? 1 : 0;
    dst[1] += p20_h;
    carry += dst[1] < p20_h ? 1 : 0;
    dst[1] += p21_l;
    carry += dst[1] < p21_l ? 1 : 0;

    dst[2] = p21_h + carry;
}

static inline bool leq_192(const uint64_t A[3], const uint64_t B[3]) {
    if (A[2] != B[2])
        return A[2] < B[2];
    if (A[1] != B[1])
        return A[1] < B[1];
    return A[0] <= B[0];
}

void log2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low) {
    uint64_t res[3];
    uint64_t x[3] = {0, low, high};
    uint64_t coeff[3];
    if (high < 0x4000000000000000ULL) {
        bool sign = log2_coeffs_bool_1[LOG2_COEFFS_SIZE_1 - 1];
        bool coeff_sign;
        res[0] = log2_coeffs_1[LOG2_COEFFS_SIZE_1 - 1][0];
        res[1] = log2_coeffs_1[LOG2_COEFFS_SIZE_1 - 1][1];
        res[2] = log2_coeffs_1[LOG2_COEFFS_SIZE_1 - 1][2];
        for (int i = 1; i < LOG2_COEFFS_SIZE_1; i++) {
            coeff[0] = log2_coeffs_1[LOG2_COEFFS_SIZE_1 - 1 - i][0];
            coeff[1] = log2_coeffs_1[LOG2_COEFFS_SIZE_1 - 1 - i][1];
            coeff[2] = log2_coeffs_1[LOG2_COEFFS_SIZE_1 - 1 - i][2];
            coeff_sign = log2_coeffs_bool_1[LOG2_COEFFS_SIZE_1 - 1 - i];

            umul192x128_tohi192(res, res, x + 1);
            if (sign == coeff_sign) {
                _u192add(res, coeff);
            } else {
                if (leq_192(res, coeff)) {
                    _u192sub(coeff, res);
                    res[0] = coeff[0];
                    res[1] = coeff[1];
                    res[2] = coeff[2];
                    sign = coeff_sign;
                } else {
                    _u192sub(res, coeff);
                }
            }
        }
    } else if (high < 0x8000000000000000ULL) {
        bool sign = log2_coeffs_bool_2[LOG2_COEFFS_SIZE_2 - 1];
        bool coeff_sign;
        res[0] = log2_coeffs_2[LOG2_COEFFS_SIZE_2 - 1][0];
        res[1] = log2_coeffs_2[LOG2_COEFFS_SIZE_2 - 1][1];
        res[2] = log2_coeffs_2[LOG2_COEFFS_SIZE_2 - 1][2];
        for (int i = 1; i < LOG2_COEFFS_SIZE_2; i++) {
            coeff[0] = log2_coeffs_2[LOG2_COEFFS_SIZE_2 - 1 - i][0];
            coeff[1] = log2_coeffs_2[LOG2_COEFFS_SIZE_2 - 1 - i][1];
            coeff[2] = log2_coeffs_2[LOG2_COEFFS_SIZE_2 - 1 - i][2];
            coeff_sign = log2_coeffs_bool_2[LOG2_COEFFS_SIZE_2 - 1 - i];

            umul192x128_tohi192(res, res, x + 1);
            if (sign == coeff_sign) {
                _u192add(res, coeff);
            } else {
                if (leq_192(res, coeff)) {
                    _u192sub(coeff, res);
                    res[0] = coeff[0];
                    res[1] = coeff[1];
                    res[2] = coeff[2];
                    sign = coeff_sign;
                } else {
                    _u192sub(res, coeff);
                }
            }
        }
    } else if (high < 0xC000000000000000ULL) {
        bool sign = log2_coeffs_bool_3[LOG2_COEFFS_SIZE_3 - 1];
        bool coeff_sign;
        res[0] = log2_coeffs_3[LOG2_COEFFS_SIZE_3 - 1][0];
        res[1] = log2_coeffs_3[LOG2_COEFFS_SIZE_3 - 1][1];
        res[2] = log2_coeffs_3[LOG2_COEFFS_SIZE_3 - 1][2];
        for (int i = 1; i < LOG2_COEFFS_SIZE_3; i++) {
            coeff[0] = log2_coeffs_3[LOG2_COEFFS_SIZE_3 - 1 - i][0];
            coeff[1] = log2_coeffs_3[LOG2_COEFFS_SIZE_3 - 1 - i][1];
            coeff[2] = log2_coeffs_3[LOG2_COEFFS_SIZE_3 - 1 - i][2];
            coeff_sign = log2_coeffs_bool_3[LOG2_COEFFS_SIZE_3 - 1 - i];

            umul192x128_tohi192(res, res, x + 1);
            if (sign == coeff_sign) {
                _u192add(res, coeff);
            } else {
                if (leq_192(res, coeff)) {
                    _u192sub(coeff, res);
                    res[0] = coeff[0];
                    res[1] = coeff[1];
                    res[2] = coeff[2];
                    sign = coeff_sign;
                } else {
                    _u192sub(res, coeff);
                }
            }
        }
    } else {
        bool sign = log2_coeffs_bool_4[LOG2_COEFFS_SIZE_4 - 1];
        bool coeff_sign;
        res[0] = log2_coeffs_4[LOG2_COEFFS_SIZE_4 - 1][0];
        res[1] = log2_coeffs_4[LOG2_COEFFS_SIZE_4 - 1][1];
        res[2] = log2_coeffs_4[LOG2_COEFFS_SIZE_4 - 1][2];
        for (int i = 1; i < LOG2_COEFFS_SIZE_4; i++) {
            coeff[0] = log2_coeffs_4[LOG2_COEFFS_SIZE_4 - 1 - i][0];
            coeff[1] = log2_coeffs_4[LOG2_COEFFS_SIZE_4 - 1 - i][1];
            coeff[2] = log2_coeffs_4[LOG2_COEFFS_SIZE_4 - 1 - i][2];
            coeff_sign = log2_coeffs_bool_4[LOG2_COEFFS_SIZE_4 - 1 - i];

            umul192x128_tohi192(res, res, x + 1);
            if (sign == coeff_sign) {
                _u192add(res, coeff);
            } else {
                if (leq_192(res, coeff)) {
                    _u192sub(coeff, res);
                    res[0] = coeff[0];
                    res[1] = coeff[1];
                    res[2] = coeff[2];
                    sign = coeff_sign;
                } else {
                    _u192sub(res, coeff);
                }
            }
        }
    }
    dst[0] = (res[1] << 1) | (res[0] >> 63);
    dst[1] = (res[2] << 1) | (res[1] >> 63);
}

void exp2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low) {
    uint64_t res[3];
    uint64_t x[2] = {low, high};
    uint64_t coeff[3];
    res[0] = exp2_coeffs[EXP2_COEFFS_SIZE - 1][0];
    res[1] = exp2_coeffs[EXP2_COEFFS_SIZE - 1][1];
    res[2] = exp2_coeffs[EXP2_COEFFS_SIZE - 1][2];
    for (int i = 1; i < EXP2_COEFFS_SIZE; i++) {
        coeff[0] = exp2_coeffs[EXP2_COEFFS_SIZE - 1 - i][0];
        coeff[1] = exp2_coeffs[EXP2_COEFFS_SIZE - 1 - i][1];
        coeff[2] = exp2_coeffs[EXP2_COEFFS_SIZE - 1 - i][2];
        umul192x128_tohi192(res, res, x);
        _u192add(res, coeff);
    }
    dst[0] = res[1];
    dst[1] = res[2];
}
#endif // 0

static inline void umul128x64_tohi128(uint64_t dst[2], const uint64_t i128[2], uint64_t i64) {
    uint64_t a0 = i128[0], a1 = i128[1];
    uint64_t b0 = i64;

    uint64_t p0l, p0h, p1l, p1h;

    _umul64to128_(a0, b0, &p0l, &p0h);
    _umul64to128_(a1, b0, &p1l, &p1h);
    /*
    | res0 | res1 | res2 | res3 |
           |  p0l |  p0h |
                  |  p1l |  p1h |
    */

    uint64_t carry;
    dst[0] = p0h + p1l;
    carry = dst[0] < p1l ? 1 : 0;
    dst[1] = p1h + carry;
}

#define LOG2_COEFFS_SIZE_64BIT 27
static const uint64_t log2_coeffs_64bit[][2] = {
    {0x00c0edd0692e4ed8ULL, 0x0000000000000000ULL}, {0x730606fe72494de1ULL, 0xb8aa3b295c17f0b7ULL},
    {0x395e32f103c4c327ULL, 0x5c551d94ae0bf448ULL}, {0x2257e76bc4c01bfaULL, 0x3d8e13b874066dbaULL},
    {0x4e99045f8476e263ULL, 0x2e2a8eca56b57f8dULL}, {0x6f44d8eced57c597ULL, 0x24eed8a1d523a64eULL},
    {0xaa4857737aad2f95ULL, 0x1ec709db5fc76232ULL}, {0x697ae6be4110d3deULL, 0x1a61761d41600e51ULL},
    {0x3f24c15e73563c74ULL, 0x171546cbe6911c51ULL}, {0x490f2bb66aed8eafULL, 0x1484abe4aefc838bULL},
    {0xfa3fefcf0da9b0b4ULL, 0x127746a483b98dacULL}, {0xcc041a6ea38888e6ULL, 0x10c8d2fa52f32e98ULL},
    {0xa2c3e0d50135e7efULL, 0x0f5fac12299b3198ULL}, {0x6c010c3b0db0362cULL, 0x0e25d29e656b5eceULL},
    {0x6be0da84b6f84ceaULL, 0x0d023369414ce836ULL}, {0x65e09ce10396d65cULL, 0x0bd37a7a39e13193ULL},
    {0xfefc3486f6d07088ULL, 0x0a722bf4bb85ca7dULL}, {0x446335c709213d24ULL, 0x08c00f58b3b07ae3ULL},
    {0x052b3f1e3af31684ULL, 0x06c0d160e8337874ULL}, {0xd8f7400ea60d1717ULL, 0x04a80179ae06330bULL},
    {0x6a32c138711091bbULL, 0x02c797f4effd244cULL}, {0xaa7b2595139c2a28ULL, 0x016402da3403615aULL},
    {0xe5b9531289e4706dULL, 0x008fe549277ce4caULL}, {0xe712330d83ceca31ULL, 0x002cda33070169aeULL},
    {0x64a7bafd0d66466bULL, 0x000a1247c0b5618fULL}, {0x5b4fe80fe1492e77ULL, 0x000171f3a2b4884cULL},
    {0x2d69f0b0e5852210ULL, 0x0000196a11aa0f6eULL}};

static const bool log2_coeffs_bool_64bit[] = {true,  true,  false, true,  false, true,  false, true,  false,
                                              true,  false, true,  false, true,  false, true,  false, true,
                                              false, true,  false, true,  false, true,  false, true,  false};

#define EXP2_COEFFS_SIZE_64BIT 14
static const uint64_t exp2_coeffs_64bit[][2] = {
    {0xfcb28ffe31b143c6ULL, 0xffffffffffffffffULL}, {0xd850edbecbb8c1ddULL, 0xb17217f7d1cf79b0ULL},
    {0x6556bcaed3ec74ccULL, 0x3d7f7bff058b1c08ULL}, {0x78b8db950f907aa4ULL, 0x0e35846b82508095ULL},
    {0xad782ad1b9e3de94ULL, 0x0276556df7481954ULL}, {0x76fd11d19068802dULL, 0x005761ff9e37415aULL},
    {0x1b86da681b1de67cULL, 0x000a1848977ce954ULL}, {0x150f90708ddbf23eULL, 0x0000ffe5ff231659ULL},
    {0x0efdbc395b4a1d53ULL, 0x0000162bffca4fa1ULL}, {0x25ee81bbb3b2d572ULL, 0x000001b52942dd28ULL},
    {0x571cc36e53b663daULL, 0x0000001e48246079ULL}, {0xab3e5be2ef0df735ULL, 0x00000001ecba3c2eULL},
    {0x31b79192ef6e7397ULL, 0x000000001a27ab70ULL}, {0x8b6ce89b1e4dd1f9ULL, 0x0000000002221a73ULL}};

uint64_t log2_fixed_64(uint64_t x) {
    uint64_t res[2];
    uint64_t coeff[2];
    bool sign = log2_coeffs_bool_64bit[LOG2_COEFFS_SIZE_64BIT - 1];
    bool coeff_sign;
    res[0] = log2_coeffs_64bit[LOG2_COEFFS_SIZE_64BIT - 1][0];
    res[1] = log2_coeffs_64bit[LOG2_COEFFS_SIZE_64BIT - 1][1];
    for (int i = 1; i < LOG2_COEFFS_SIZE_64BIT; i++) {
        coeff[0] = log2_coeffs_64bit[LOG2_COEFFS_SIZE_64BIT - 1 - i][0];
        coeff[1] = log2_coeffs_64bit[LOG2_COEFFS_SIZE_64BIT - 1 - i][1];
        coeff_sign = log2_coeffs_bool_64bit[LOG2_COEFFS_SIZE_64BIT - 1 - i];
        umul128x64_tohi128(res, res, x);

        if (sign == coeff_sign) {
            _u128add(res, res, coeff);
        } else {
            if (_u128cmp(res, coeff)) {
                _u128sub(res, coeff, res);
                sign = coeff_sign;
            } else {
                _u128sub(res, res, coeff);
            }
        }
    }
    return (res[1] << 1) | (res[0] >> 63);
}

uint64_t exp2_fixed_64(uint64_t x) {
    uint64_t res[2];
    uint64_t coeff[2];
    res[0] = exp2_coeffs_64bit[EXP2_COEFFS_SIZE_64BIT - 1] [0];
    res[1] = exp2_coeffs_64bit[EXP2_COEFFS_SIZE_64BIT - 1] [1];
    for (int i = 1; i < EXP2_COEFFS_SIZE_64BIT; i++) {
        coeff[0] = exp2_coeffs_64bit[EXP2_COEFFS_SIZE_64BIT - 1 - i][0];
        coeff[1] = exp2_coeffs_64bit[EXP2_COEFFS_SIZE_64BIT - 1 - i][1];
        umul128x64_tohi128(res, res, x);
        _u128add(res, res, coeff);
    }
    return res[1];
}
