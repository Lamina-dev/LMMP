/**
 *  Copyright (C) 2026 HJimmyK(Jericho Knox)
 *
 *  This file is part of LMMP.
 *
 *  LMMP is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License (LGPL) as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed WITHOUT ANY WARRANTY.
 *
 *  See <https://www.gnu.org/licenses/>.
 */

#include "../../../include/lmmp/impl/log2_exp2.h"
#include "../../../include/lmmp/impl/longlong.h"

/*
    记 B = 2^64，x 为 [0,1) 上的小数（64bit 或 128bit 定点），本文件计算
    log2(1+x) 与 2^x - 1 的定点值。结构为“查表约简 + 短泰勒级数”，
    查表索引为7bit，级数项数（log2 19/10 项，exp2 13/7 项）

    约简恒等式（i 为索引，xl 为剩余低位）：

      log2(1+x) = L[i] + log2(1+u)，u = xl/(2^k*(128+i)) < 2^-7
      2^x - 1   = Q * (2^t)，Q = 2^(i/128)，t < 2^-7

    u 由倒数表（2^128/(128+i) 的定点值 lg2_R128）一次乘法获得。级数
    按奇偶次幂分组（w = u^2 或 t^2，均为正系数规避符号分支）：

        log2(1+u) = u*B(w) - w*A(w)
        2^t - 1   = t*B(w) + w*A(w)

    项数由首个截断项 < 2^-66（64bit 输出）/ 2^-130（128bit 输出）
    确定。lg2 系数按 2^191 尺度存储（c1 = 1/ln2 > 1 超出定点范围），
    链与收尾按半尺度（2^63/2^127/2^191）推进，最终结果左移 1 位
    恢复；xp2 系数均 < 1，按满尺度 2^192 存储。

    舍入控制：约简变量 u 的定点化为误差主源（~0.7 ulp），其余各级
    floor 系统性偏负，故 64bit 版 B/A 链取 floor 而收尾以 +2^63 /
    舍入位中心化，exp2 的表合成（T16 四舍五入存、T8 运行时舍入取
    高位）与 s、Q*s 项亦取舍入值。实测全区间误差 |e| <= 2 ulp，
    接口承诺 |e| <= 2。

    常量表由纯整数高精度脚本离线生成（atanh/exp 级数求值，ln2 取
    640bit 定点），每张表首行为 i=0。
*/

static const uint64_t lg2_L192[128][3] = {
    {0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull},
    {0x399bec91e4cb483cull, 0xf1c6f6002f29e888ull, 0x02dfca16dde10a2full},
    {0x82a5dbcb6ed52c01ull, 0x9b89f8846042be51ull, 0x05b9e5a170b48a62ull},
    {0xb57feea297d30b99ull, 0xc0a2827d49a3a979ull, 0x088e68ea899a0976ull},
    {0x6366f25ae368399cull, 0x9b03784b5be08490ull, 0x0b5d69bac77ec398ull},
    {0xd2b182fdaac37535ull, 0x7c7c34f31dc4142bull, 0x0e26fd5c8555af7aull},
    {0xb9150f3c7ddd2235ull, 0xcf74bab999217066ull, 0x10eb389fa29f9ab3ull},
    {0xd48d0be3c5dbc5fbull, 0x04d1121b4a6276a6ull, 0x13aa2fdd27f1c2d8ull},
    {0xca507cfab1d27f98ull, 0xcc53826144575ac3ull, 0x1663f6fac913167cull},
    {0x8d644e6e1ef970d7ull, 0x7232494db3a3a320ull, 0x1918a16e46335aaeull},
    {0x641c50810e3b820cull, 0xb2c5a6e5197ab879ull, 0x1bc84240adabba63ull},
    {0x543b8b67096d189eull, 0xbdb5d9dc29f204eaull, 0x1e72ec117fa5b21cull},
    {0x9e77fd40e0401d05ull, 0x4f78dfa14aa5157aull, 0x2118b119b4f3c72cull},
    {0x0fba915683493aa8ull, 0x48a860f072c2aeb5ull, 0x23b9a32eaa56f6bdull},
    {0x2a7e25898ddd2376ull, 0xa3e580eb4e974c9bull, 0x2655d3c4f15c343eull},
    {0x5b678f89d1150023ull, 0x71d282c87ed827ddull, 0x28ed53f307ee9a62ull},
    {0x49e7cd4744b36080ull, 0x401624140d175ba2ull, 0x2b803473f7ad0f3full},
    {0x6b89a4670d7cb441ull, 0x8039f5aefcf6d452ull, 0x2e0e85a9de04fe53ull},
    {0xfad97695fb7fb9a6ull, 0xa4491dcec752ae1eull, 0x309857a05e0765fbull},
    {0x96813133a53c7fd5ull, 0xf59d19522e56fe5full, 0x331dba0efce1be05ull},
    {0x41057dd4287e22f1ull, 0xc23d9780306c696aull, 0x359ebc5b69d927dfull},
    {0x82482b9b4a45aa67ull, 0xc4db31339fde86bdull, 0x381b6d9bb29bdc81ull},
    {0xf05696955ff71ab5ull, 0xe96aca04740a8837ull, 0x3a93dc9864b2df91ull},
    {0xf5567a7d00d760bcull, 0x93e7aa3bdf8707e4ull, 0x3d0817ce9cd4998full},
    {0x99eaddb5a9520d6aull, 0x51b3314f09de6be4ull, 0x3f782d7204d01447ull},
    {0x14384a41f685e019ull, 0x0c69a675516eb666ull, 0x41e42b6ec0c025bcull},
    {0x7739b0ff427ff126ull, 0x25c169e5693a7f06ull, 0x444c1f6b4c2dd72cull},
    {0x6c7349cb4b3fbdacull, 0x31ce1b7e32868187ull, 0x46b016ca47c1c14aull},
    {0x41e86a7632602708ull, 0x16e52e91300efeefull, 0x49101eac381ce609ull},
    {0x716701717c89c120ull, 0x4de8f631bcf371dcull, 0x4b6c43f1366abdbcull},
    {0x41376f4612d4b0b9ull, 0x44cdb2581fb9186eull, 0x4dc4933a9337b366ull},
    {0xe401e28a8e592d7bull, 0xcbcd10948cd497bcull, 0x501918ec6c1125d6ull},
    {0x65b157f8deceb53aull, 0x24afdbfd36bf6d33ull, 0x5269e12f346e2bf9ull},
    {0x77ef0f256d533996ull, 0x3d83987f26d4b2efull, 0x54b6f7f1325acdf7ull},
    {0x93cf9a8e8966c101ull, 0x802c48281a2eb744ull, 0x570068e7ef5a1e7eull},
    {0x17e2d5cc67da36bfull, 0x89e3b7227a621d2dull, 0x59463f919dee9b94ull},
    {0x9f890503285f6d81ull, 0x35482d13dc0f110cull, 0x5b8887367433795eull},
    {0xc9184a53c236eed6ull, 0xbfb35448929ff1c3ull, 0x5dc74ae9fbecef91ull},
    {0x17b1bf9249a8e12dull, 0xbad827d37deb2236ull, 0x6002958c587150caull},
    {0xec76dc7f32fc676full, 0xd99293236a6eac43ull, 0x623a71cb82c89692ull},
    {0x5dba8beba3cb180bull, 0xcad415ae1a715618ull, 0x646eea247c5c22d2ull},
    {0x39e907a5200ced8full, 0xedb4390e5306a23cull, 0x66a008e4788cbcd2ull},
    {0x3001d4f390a134d2ull, 0xf1035e5e7b16c7f7ull, 0x68cdd829fd814275ull},
    {0xe3d2aafcee056debull, 0x91c9556316f5c786ull, 0x6af861e5fc7d2386ull},
    {0x14a805ee427197fcull, 0x51bbe3f6289e3ab7ull, 0x6d1fafdce20a8290ull},
    {0x445fadb660f84211ull, 0x10b38c8045b0a29aull, 0x6f43cba79e40c2adull},
    {0x2acc3311d107af47ull, 0xfb952bbbccc314f0ull, 0x7164beb4a56d59f9ull},
    {0x04295539bf0ed23full, 0x7428bb9e816482aeull, 0x73829248e961f325ull},
    {0x3e730bb7410e895bull, 0xfaf866415554d6bfull, 0x759d4f80cba83bf8ull},
    {0x9622f6e9e90fcef0ull, 0xec658457c40d2ec9ull, 0x77b4ff5108d9313aull},
    {0x8a375ea75157976bull, 0x46784bd1c44ccd5eull, 0x79c9aa879d534831ull},
    {0xccddb53ed88c47c7ull, 0x7806a0e4104907f9ull, 0x7bdb59cca38881f4ull},
    {0xaf992540238215bbull, 0x64c6001143d6c8d5ull, 0x7dea15a32c1b3b38ull},
    {0xb4ab6119190cd10cull, 0x31fcd0be2e188b17ull, 0x7ff5e66a0ffe6ae7ull},
    {0x55bb291a9177de92ull, 0xa1a3202b3d68f965ull, 0x81fed45cbccbf99cull},
    {0xccb0f7761e5ff9b9ull, 0xd9e0e8ac1f51e170ull, 0x8404e793fb81ea92ull},
    {0x3f2869dd6be1d1ccull, 0x12ba94db12ef0aa8ull, 0x86082806b1d532c4ull},
    {0xa6b6d5cd074cd82bull, 0xe6ed737d672bd69dull, 0x88089d8a9e4753d8ull},
    {0x2bb5d8760ecab8b3ull, 0xad29518b0252c225ull, 0x8a064fd50f2a1cf0ull},
    {0x08c388b1f2e108f3ull, 0xc74be8a299ac3183ull, 0x8c01467b94bb5275ull},
    {0xeac6aab2e0ef8a67ull, 0xa89d4ee66c3700e3ull, 0x8df988f4ae806f1dull},
    {0xa7a25932e87e788cull, 0x0aea39c22788b1baull, 0x8fef1e9874093212ull},
    {0xff9c35ae8820c2a4ull, 0x76630d4c409dd917ull, 0x91e20ea1393e4040ull},
    {0xf85a744684c6676aull, 0xc277071d74f9d751ull, 0x93d2602c2e5fc02bull},
    {0x24f3e6a3a259b040ull, 0xa00b120a068badd1ull, 0x95c01a39fbd6879full},
    {0x0344f129c3187043ull, 0x3c668b4c988c5d32ull, 0x97ab43af59f930a1ull},
    {0xe6edc060bf80459dull, 0x5c902fd211010939ull, 0x9993e355a4e53643ull},
    {0xa799c26f112edc42ull, 0x3b950a8e66ce6c22ull, 0x9b79ffdb6c8b1202ull},
    {0x718d4a9002e2cf95ull, 0x5592074827cb508eull, 0x9d5d9fd5010b3666ull},
    {0x1817bfeddee4a963ull, 0x153d6d2a96369ad8ull, 0x9f3ec9bcfb80b357ull},
    {0x885ad8fe85c1e9dcull, 0x3b0e8a55626c3261ull, 0xa11d83f4c3554b38ull},
    {0x75be89f0841ae837ull, 0xbfff0133975541c7ull, 0xa2f9d4c51039c526ull},
    {0xcb62aff1bd9d6a74ull, 0x495fb7fa6d7eda66ull, 0xa4d3c25e68dc57f2ull},
    {0xde08f5e02036d275ull, 0x6f7fccc39fad1e37ull, 0xa6ab52d99e762253ull},
    {0x4e8c939a60252b34ull, 0x4a49bc591348f145ull, 0xa8808c384547c6efull},
    {0x3e9ef1b6301f66d1ull, 0x86531d55da1d0f66ull, 0xaa5374652a1c6d8dull},
    {0xef44639e542c2fd8ull, 0x6c5e946b4ae30894ull, 0xac241134c4e99e1cull},
    {0x053a5cfc072e22bbull, 0x59f8091112ce7e40ull, 0xadf26865a8a1a557ull},
    {0xf236084d1d68fe08ull, 0x58d602e66b04d3b5ull, 0xafbe7fa0f04d75c6ull},
    {0x89103724b095324dull, 0x52d0b8ef2006664aull, 0xb1885c7aa9824203ull},
    {0x1cf483d2900676c7ull, 0x76da1c872983511eull, 0xb35004723c465e69ull},
    {0xd85de96cea6096c6ull, 0x4cab97905f3342a3ull, 0xb5157cf2d0785040ull},
    {0xc36be3e48299cd45ull, 0xef83f1ab5130c34bull, 0xb6d8cb53b0ca4ecbull},
    {0x6c7cb214a0b718fcull, 0xb3fdee409b935e71ull, 0xb899f4d8ab63df28ull},
    {0xbf0dfbe6ebff7d3bull, 0x2bc1fe8a8648e9ebull, 0xba58feb2703a9e37ull},
    {0x4f720c2d3036d3b6ull, 0x43f092f55522fa6cull, 0xbc15edfeed32bbdeull},
    {0x6ff4835615edb5a9ull, 0x55bbf90ce3f6815bull, 0xbdd0c7c9a817204full},
    {0x7a595de721404d36ull, 0x766bbff35f5da7eaull, 0xbf89910c1678ae89ull},
    {0x6edbb3eae70d10c1ull, 0xe021361e13a30973ull, 0xc1404eadf38396deull},
    {0x3f88971a6de2703dull, 0x37e743250facbc9bull, 0xc2f5058593d93084ull},
    {0xbffcebbd72cbc1dcull, 0x75163ec8d56242f8ull, 0xc4a7ba58377c5a03ull},
    {0x1fcd5d399dd969e6ull, 0x44542fd8cdde5bf0ull, 0xc65871da59dded9bull},
    {0xa42463b01fdd3e95ull, 0x1fa8423e8c1443f2ull, 0xc80730b0001667f2ull},
    {0xe74500cd41d8f65full, 0x432d9ee86ddaabe1ull, 0xc9b3fb6d055974e6ull},
    {0x65f96477cad7d331ull, 0x6248a98a36f8173bull, 0xcb5ed69565afaf7full},
    {0xd82d235c686444f3ull, 0xab9a55e97e60e631ull, 0xcd07c69d87027ef4ull},
    {0x38c6a548017167caull, 0x2ac903a413e5a847ull, 0xceaecfea80859b33ull},
    {0x154a7d390250caf5ull, 0x8975dc0e7a963609ull, 0xd053f6d260896731ull},
    {0x3b8ac4f583573138ull, 0xcc68d510b4a2b099ull, 0xd1f73f9c70c0f683ull},
    {0xa6a63f977ff97e1bull, 0xb051ff7044a68a7full, 0xd398ae8179063deaull},
    {0xbedec4594babbdabull, 0xf1be4359106a19b5ull, 0xd53847ac00a69be6ull},
    {0x70bb9fab077c80acull, 0xb16b0d7b9fbfa949ull, 0xd6d60f388e41968aull},
    {0xa4d9c1d64ab08706ull, 0x376a70d849ae77dbull, 0xd8720935e6435ebdull},
    {0x9c2d97a2e4d9a166ull, 0xc5cc7bef6fc62cd7ull, 0xda0c39a548045ecbull},
    {0x05d84c6e2eadff07ull, 0x5b8a19b1c637671full, 0xdba4a47aa996d25aull},
    {0x7268dda91538c3c0ull, 0xc3f6f0a574650654ull, 0xdd3b4d9cf24b2077ull},
    {0x66dc5119d4b9d748ull, 0xb6f0409b369aacc0ull, 0xded038e633f36da8ull},
    {0x507802abbfbe3fa2ull, 0xcd4d2ae3a2f66e17ull, 0xe0636a23e2ee9b16ull},
    {0x712f104646b61eb0ull, 0x4c5a724dbd8180f8ull, 0xe1f4e5170d02a99bull},
    {0x662b55e9b52e60faull, 0xe4d8c4622644c63full, 0xe384ad748f0e3b05ull},
    {0x03172242b343e763ull, 0xf71c8605583d030bull, 0xe512c6e54998b1afull},
    {0x3d1b54ce1c429653ull, 0x1ba488a625775d26ull, 0xe69f350654483612ull},
    {0x8aa53e9c8128657aull, 0xc4baee073d4b1b04ull, 0xe829fb693044b398ull},
    {0x98d13ab7bd9975ceull, 0xc1bd6da56c7255d2ull, 0xe9b31d93f98ea94bull},
    {0x7ce6176e821d12b7ull, 0xf5f0cc82aaa9ad7eull, 0xeb3a9f01975077f1ull},
    {0xb8c381322bc07142ull, 0x20375a3220ba6515ull, 0xecc08321eb30a61eull},
    {0xa7bd10d27c064978ull, 0x39d5d6a218c6339full, 0xee44cd59ffab62f3ull},
    {0xd73ea288e1b184cfull, 0x7a41e3455e8abdc1ull, 0xefc781043579625full},
    {0xc47ceba6cab91dc2ull, 0xd5533f1de29abeddull, 0xf148a170700a00fdull},
    {0xb6df618839588432ull, 0xc88d4dd63361bd02ull, 0xf2c831e4411672b0ull},
    {0x06c1f1d26c710872ull, 0x0d1e3f80fbc71454ull, 0xf446359b13539551ull},
    {0x3ca5a635ec02916dull, 0x5ae339dd8476d007ull, 0xf5c2afc65447d86aull},
    {0x311407ea9c6c1faeull, 0x6e0f93f7a43e479aull, 0xf73da38d9d4a83ebull},
    {0xa892ef2e78e86373ull, 0x0c94610afb5eac21ull, 0xf8b7140edbb181d9ull},
    {0x82ae728f4624c84bull, 0x6adf27b820fd03e9ull, 0xfa2f045e7832aa72ull},
    {0x7d9b7594acf05b27ull, 0x0db2fb1c6843e167ull, 0xfba577877d7d6ebdull},
    {0x56b294b3a012f0dfull, 0x945cf6ba73d491eaull, 0xfd1a708bbe119b14ull},
    {0x54f5bb9732fae513ull, 0x910e706881a275c8ull, 0xfe8df263f957ca15ull},
};

static const uint64_t lg2_R128[128][2] = {
    {0x0000000000000000ull, 0x0200000000000000ull},{0xfc07f01fc07f01fcull, 0x01fc07f01fc07f01ull},
    {0x1f81f81f81f81f81ull, 0x01f81f81f81f81f8ull},{0x7f05dcd30dadec75ull, 0x01f44659e4a42715ull},
    {0x1f07c1f07c1f07c1ull, 0x01f07c1f07c1f07cull},{0xb301ecc07b301eccull, 0x01ecc07b301ecc07ull},
    {0xa07a44c6afc2dd9cull, 0x01e9131abf0b7672ull},{0xc901e573ac901e57ull, 0x01e573ac901e573aull},
    {0xe1e1e1e1e1e1e1e1ull, 0x01e1e1e1e1e1e1e1ull},{0x701de5d6e3f8868aull, 0x01de5d6e3f8868a4ull},
    {0x6076b981dae6076bull, 0x01dae6076b981daeull},{0x17f14424d5a3e9e6ull, 0x01d77b654b82c339ull},
    {0x1d41d41d41d41d41ull, 0x01d41d41d41d41d4ull},{0x2d63dbb01d0cb58full, 0x01d0cb58f6ec0743ull},
    {0x12073615a240e6c2ull, 0x01cd85689039b0adull},{0x1ca4b3055ee19101ull, 0x01ca4b3055ee1910ull},
    {0x1c71c71c71c71c71ull, 0x01c71c71c71c71c7ull},{0xc3f8f01c3f8f01c3ull, 0x01c3f8f01c3f8f01ull},
    {0x0381c0e070381c0eull, 0x01c0e070381c0e07ull},{0xae26501bdd2b8994ull, 0x01bdd2b899406f74ull},
    {0x14c1bacf914c1bacull, 0x01bacf914c1bacf9ull},{0xaf3f920a4f089731ull, 0x01b7d6c3dda338b2ull},
    {0x1b4e81b4e81b4e81ull, 0x01b4e81b4e81b4e8ull},{0x1b2036406c80d901ull, 0x01b2036406c80d90ull},
    {0xbca1af286bca1af2ull, 0x01af286bca1af286ull},{0x5701ac5701ac5701ull, 0x01ac5701ac5701acull},
    {0x1a98ef606a63bd81ull, 0x01a98ef606a63bd8ull},{0x1a6d01a6d01a6d01ull, 0x01a6d01a6d01a6d0ull},
    {0x1a41a41a41a41a41ull, 0x01a41a41a41a41a4ull},{0x16d3f97a4b01a16dull, 0x01a16d3f97a4b01aull},
    {0xd2a2067b23a5440cull, 0x019ec8e951033d91ull},{0xc2d14ee4a1019c2dull, 0x019c2d14ee4a1019ull},
    {0x9999999999999999ull, 0x0199999999999999ull},{0xc065c393e032e1c9ull, 0x01970e4f80cb8727ull},
    {0x522c3f35ba781948ull, 0x01948b0fcd6e9e06ull},{0x59857f36f825b178ull, 0x01920fb49d0e228dull},
    {0x18f9c18f9c18f9c1ull, 0x018f9c18f9c18f9cull},{0x18d3018d3018d301ull, 0x018d3018d3018d30ull},
    {0x3784a062b2e43dafull, 0x018acb90f6bf3a9aull},{0x4b1d20310dcbe157ull, 0x01886e5f0abb0499ull},
    {0x1861861861861861ull, 0x0186186186186186ull},{0x8e63f9f0da215350ull, 0x0183c977ab2bedd2ull},
    {0x8181818181818181ull, 0x0181818181818181ull},{0xfd017f405fd017f4ull, 0x017f405fd017f405ull},
    {0x7d05f417d05f417dull, 0x017d05f417d05f41ull},{0x458c93fa14b77dc7ull, 0x017ad2208e0ecc35ull},
    {0x78a4c8178a4c8178ull, 0x0178a4c8178a4c81ull},{0x1767dce434a9b101ull, 0x01767dce434a9b10ull},
    {0x1745d1745d1745d1ull, 0x01745d1745d1745dull},{0x5c90a1fd1b7af017ull, 0x01724287f46debc0ull},
    {0xe05c0b81702e05c0ull, 0x01702e05c0b81702ull},{0xb1573d7f48f044a5ull, 0x016e1f76b4337c6cull},
    {0x16c16c16c16c16c1ull, 0x016c16c16c16c16cull},{0x3e3b673fa57b0cbaull, 0x016a13cd15372904ull},
    {0x1681681681681681ull, 0x0168168168168168ull},{0x1661ec6a5122f901ull, 0x01661ec6a5122f90ull},
    {0xc8590b21642c8590ull, 0x01642c8590b21642ull},{0x7701623fa7701623ull, 0x01623fa7701623faull},
    {0x1605816058160581ull, 0x0160581605816058ull},{0xbb8d015e75bb8d01ull, 0x015e75bb8d015e75ull},
    {0x620ae4c415c9882bull, 0x015c9882b9310572ull},{0x6b015ac056b015acull, 0x015ac056b015ac05ull},
    {0x308158ed2308158eull, 0x0158ed2308158ed2ull},{0x22d9218202ae3da7ull, 0x01571ed3c506b39aull},
    {0x5555555555555555ull, 0x0155555555555555ull},{0x6f6b70bf01539094ull, 0x015390948f40feacull},
    {0xd07eae2f8151d07eull, 0x0151d07eae2f8151ull},{0x1501501501501501ull, 0x0150150150150150ull},
    {0x829cbc14e5e0a72full, 0x014e5e0a72f05397ull},{0x4f44df833facd51dull, 0x014cab88725af6e7ull},
    {0x14afd6a052bf5a81ull, 0x014afd6a052bf5a8ull},{0xa21727e120292a73ull, 0x0149539e3b2d066eull},
    {0x147ae147ae147ae1ull, 0x0147ae147ae147aeull},{0xc051832f1fd73e68ull, 0x01460cbc7f5cf9a1ull},
    {0xe41e6a74981446f8ull, 0x01446f86562d9faeull},{0xf9b1d0142d6625d5ull, 0x0142d6625d51f86eull},
    {0x4141414141414141ull, 0x0141414141414141ull},{0x13fb013fb013fb01ull, 0x013fb013fb013fb0ull},
    {0xc45979c95204f88bull, 0x013e22cbce4a9027ull},{0x404f265691eeaf9dull, 0x013c995a47babe74ull},
    {0x13b13b13b13b13b1ull, 0x013b13b13b13b13bull},{0x71e9f3c04e6470b0ull, 0x013991c2c187f633ull},
    {0x1381381381381381ull, 0x0138138138138138ull},{0x53b7342bad7f64b3ull, 0x013698df3de07479ull},
    {0x521cfb2b78c13521ull, 0x013521cfb2b78c13ull},{0x0c04ceb916d5ef2cull, 0x0133ae45b57bcb1eull},
    {0x6e0e5aea77a04c8full, 0x01323e34a2b10bf6ull},{0x30d190130d190130ull, 0x0130d190130d1901ull},
    {0xbda12f684bda12f6ull, 0x012f684bda12f684ull},{0x12e025c04b809701ull, 0x012e025c04b80970ull},
    {0x4d812c9fb4d812c9ull, 0x012c9fb4d812c9fbull},{0xad012b404ad012b4ull, 0x012b404ad012b404ull},
    {0x129e4129e4129e41ull, 0x0129e4129e4129e4ull},{0x8b01288b01288b01ull, 0x01288b01288b0128ull},
    {0xb88127350b881273ull, 0x0127350b88127350ull},{0x3840497889c2024bull, 0x0125e22708092f11ull},
    {0x9249249249249249ull, 0x0124924924924924ull},{0x123456789abcdf01ull, 0x0123456789abcdf0ull},
    {0x21fb78121fb78121ull, 0x0121fb78121fb781ull},{0x75494dd0a2657f6full, 0x0120b470c67c0d88ull},
    {0x7dc11f7047dc11f7ull, 0x011f7047dc11f704ull},{0x313011e2ef3b3fb8ull, 0x011e2ef3b3fb8744ull},
    {0x06ada2811cf06adaull, 0x011cf06ada2811cfull},{0x11bb4a4046ed2901ull, 0x011bb4a4046ed290ull},
    {0x1a7b9611a7b9611aull, 0x011a7b9611a7b961ull},{0x46514e02328a7011ull, 0x0119453808ca29c0ull},
    {0x1181181181181181ull, 0x0118118118118118ull},{0xb4d583d0116e0689ull, 0x0116e0689427378eull},
    {0x456c797dd49c3411ull, 0x0115b1e5f75270d0ull},{0x8c6c045217c382b3ull, 0x011485f0e0acd3b6ull},
    {0x5c81135c81135c81ull, 0x01135c81135c8113ull},{0xa0ab617909a3e202ull, 0x0112358e75d30336ull},
    {0x1111111111111111ull, 0x0111111111111111ull},{0xef010fef010fef01ull, 0x010fef010fef010full},
    {0xe26152832c6e043bull, 0x010ecf56be69c8fdull},{0x8c1d7f7926fabb85ull, 0x010db20a88f46959ull},
    {0x10c9714fbcda3ac1ull, 0x010c9714fbcda3acull},{0x354a3010b7e6ec25ull, 0x010b7e6ec259dc79ull},
    {0x10a6810a6810a681ull, 0x010a6810a6810a68ull},{0x39010953f3901095ull, 0x010953f39010953full},
    {0x1084210842108421ull, 0x0108421084210842ull},{0xcfadc041cc98291full, 0x01073260a47f7c66ull},
    {0x76c8b4395810624dull, 0x010624dd2f1a9fbeull},{0x465fdf5cd0105197ull, 0x0105197f7d734041ull},
    {0x1041041041041041ull, 0x0104104104104104ull},{0xeecc652f8eac040cull, 0x0103091b51f5e1a4ull},
    {0x0204081020408102ull, 0x0102040810204081ull},{0x0101010101010101ull, 0x0101010101010101ull},
};

static const uint64_t xp2_T16[16][2] = {
    {0x0000000000000000ull, 0x0000000000000000ull},    {0x8b92b71842a98364ull, 0x0b5586cf9890f629ull},
    {0xf7c8c50eb14a7920ull, 0x172b83c7d517adcdull},    {0x1fadb1c15cb593b0ull, 0x2387a6e75623866cull},
    {0x8d5a46305c85ededull, 0x306fe0a31b7152deull},    {0x41223e13d773fba3ull, 0x3dea64c12342235bull},
    {0x397afec42e20e036ull, 0x4bfdad5362a271d4ull},    {0x93015191eb345d89ull, 0x5ab07dd48542958cull},
    {0xb2fb1366ea957d3eull, 0x6a09e667f3bcc908ull},    {0x51023f6cda1f5ef4ull, 0x7a11473eb0186d7dull},
    {0x7c55a192c9bb3e6full, 0x8ace5422aa0db5baull},    {0xc46b071f2be58ddbull, 0x9c49182a3f0901c7ull},
    {0x734d1773205a7fbcull, 0xae89f995ad3ad5e8ull},    {0x0cb12a091ba66794ull, 0xc199bdd85529c222ull},
    {0xa05aeb66e0dca9f6ull, 0xd5818dcfba48725dull},    {0xf73a18f5db301f87ull, 0xea4afa2a490d9858ull},
};

static const uint64_t xp2_T8[8][3] = {
    {0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull},
    {0x03ec04c360be2404ull, 0x4a66ae336dcdfa40ull, 0x0163da9fb33356d8ull},
    {0x3d70a2cabc5cb89cull, 0xf7caca4f7a29bde9ull, 0x02c9a3e778060ee6ull},
    {0x40bb4bfc05af6456ull, 0x38f9a20da47e6ed0ull, 0x04315e86e7f84bd7ull},
    {0x53e3495f7df4baf8ull, 0x7c548eb68ca417feull, 0x059b0d31585743aeull},
    {0xcb8b092ac75e3117ull, 0xc403a9d87b27ed07ull, 0x0706b29ddf6ddc6dull},
    {0x1b2d6829d8993a0dull, 0x35f25d9427fa2b04ull, 0x0874518759bc808cull},
    {0x2e023da730e7fccbull, 0x1e060c584d6b74baull, 0x09e3ecac6f383452ull},
};

// log2 级数系数 c_m = floor(2^191/(m*ln2))，m=1..22（半尺度，含符号吸收）
static const uint64_t lg2_c[19][3] = {
    {0xeb577aa8dd695a58ull, 0xbe87fed0691d3e88ull, 0xb8aa3b295c17f0bbull},
    {0x75abbd546eb4ad2cull, 0xdf43ff68348e9f44ull, 0x5c551d94ae0bf85dull},
    {0x4e727e3849cdc8c8ull, 0x3f82aa45785f14d8ull, 0x3d8e13b87407fae9ull},
    {0x3ad5deaa375a5696ull, 0xefa1ffb41a474fa2ull, 0x2e2a8eca5705fc2eull},
    {0x2f117eee92aeababull, 0x594e6629ae9f72e8ull, 0x24eed8a1df37fcf2ull},
    {0x27393f1c24e6e464ull, 0x9fc15522bc2f8a6cull, 0x1ec709dc3a03fd74ull},
    {0x219ec86144339f31ull, 0x645c921dc5df9b38ull, 0x1a61762a7aded93full},
    {0x1d6aef551bad2b4bull, 0x77d0ffda0d23a7d1ull, 0x171547652b82fe17ull},
    {0xc4d0d4bd6def42edull, 0x6a80e36c7d7506f2ull, 0x1484b13d7c02a8f8ull},
    {0x1788bf77495755d5ull, 0x2ca73314d74fb974ull, 0x12776c50ef9bfe79ull},
    {0x2caadc9afcdb0836ull, 0x5723a2cd20d41cf5ull, 0x10c9a84994022d28ull},
    {0x139c9f8e12737232ull, 0x4fe0aa915e17c536ull, 0x0f6384ee1d01febaull},
    {0xeab7f5be386a90cbull, 0x711e274b1bc72c31ull, 0x0e347ab4698bb00eull},
    {0x10cf6430a219cf98ull, 0xb22e490ee2efcd9cull, 0x0d30bb153d6f6c9full},
    {0x0fb07fa4db8f8e8eull, 0x731a220de4dfd0f8ull, 0x0c4f9d8b4a67fefbull},
    {0x8eb577aa8dd695a5ull, 0xbbe87fed0691d3e8ull, 0x0b8aa3b295c17f0bull},
    {0x77416191766f9be7ull, 0xa1cbc3b1e810c771ull, 0x0adcd64dba1f86a1ull},
    {0x62686a5eb6f7a176ull, 0x354071b63eba8379ull, 0x0a42589ebe01547cull},
    {0x19dc2ee077715598ull, 0x4d65793363d91e3dull, 0x09b81e0fa687ff32ull},
};

// exp2 级数系数 c_m = floor((ln2)^m/m!*2^192)，m=1..13（满尺度）
static const uint64_t xp2_c[13][3] = {
    {0x40f343267298b62dull, 0xc9e3b39803f2f6afull, 0xb17217f7d1cf79abull},
    {0x4744ea38619cd3a9ull, 0xde2d60dd92e6bf95ull, 0x3d7f7bff058b1d50ull},
    {0x4f5c47444da0110eull, 0x99d3b15d995e96f7ull, 0x0e35846b82505fc5ull},
    {0xe48f1d4a7cc7223aull, 0x39977c16a7dd58a0ull, 0x0276556df749cee5ull},
    {0xc15db29a5b9c65c3ull, 0x41c5fda69452fb0cull, 0x005761ff9e299cc4ull},
    {0x9f6629ff9988f760ull, 0xb7a58544c3591a0full, 0x000a184897c363c3ull},
    {0x959c22a5d1021fdcull, 0x34358a8e643ec734ull, 0x0000ffe5fe2c4586ull},
    {0x4b0dc341ee20f572ull, 0x23fd8ffe606da77cull, 0x0000162c0223a5c8ull},
    {0x08a319719553744cull, 0x7c3da4a70e5a4ff9ull, 0x000001b5253d395eull},
    {0x699c540c1142cae1ull, 0x8ec9f6fda1d952e7ull, 0x0000001e4cf5158bull},
    {0x2149db8f67e53838ull, 0x1bb24c0f57995e47ull, 0x00000001e8cac735ull},
    {0xa2ee61ced55dbe2cull, 0xfc2985e2b5687e17ull, 0x000000001c3bd650ull},
    {0xf71b19cdfd03e10eull, 0x166d0f96281ac300ull, 0x0000000001816193ull},
};

// dst = [a,2]*[b,2] 的最高 128bit
static inline void umul128x128_tohi128(uint64_t dst[2], const uint64_t a[2], const uint64_t b[2]) {
    __uint128_t p01 = (__uint128_t)a[0] * b[1];
    __uint128_t p10 = (__uint128_t)a[1] * b[0];
    __uint128_t p11 = (__uint128_t)a[1] * b[1];
    // w1 = p00h + p01l + p10l（不输出，仅需向 w2 的进位；首操作数提升到
    // 128bit 再求和，避免三个 64bit 操作数在 64bit 域内回卷丢进位）
    __uint128_t w1 = (__uint128_t)(uint64_t)(((__uint128_t)a[0] * b[0]) >> 64) + (uint64_t)p01 + (uint64_t)p10;
    // w2 = p01h + p10h + p11l + w1 进位
    __uint128_t w2 = (p01 >> 64) + (p10 >> 64) + (uint64_t)p11 + (uint64_t)(w1 >> 64);
    dst[0] = (uint64_t)w2;
    dst[1] = (uint64_t)(p11 >> 64) + (uint64_t)(w2 >> 64);
}

// r = x + y（128 位；调用点均为 r==x 就地模式，r==y 会丢失进位）
#define add_u128(r, x, y)                                     \
    do {                                                      \
        (r)[0] = (x)[0] + (y)[0];                             \
        (r)[1] = (x)[1] + (y)[1] + ((r)[0] < (y)[0] ? 1 : 0); \
    } while (0)

// r = x - y（128 位；同上）
#define sub_u128(r, x, y)               \
    do {                                \
        uint64_t _b_ = (x)[0] < (y)[0]; \
        (r)[0] = (x)[0] - (y)[0];       \
        (r)[1] = (x)[1] - (y)[1] - _b_; \
    } while (0)

// i += j（192 位；同上）
#define add_u192(i, j)                            \
    do {                                          \
        (i)[0] += (j)[0];                         \
        uint64_t _c_ = ((i)[0] < (j)[0]) ? 1 : 0; \
        (i)[1] += _c_;                            \
        _c_ = ((i)[1] < _c_) ? 1 : 0;             \
        (i)[1] += (j)[1];                         \
        _c_ += ((i)[1] < (j)[1]) ? 1 : 0;         \
        (i)[2] += _c_ + (j)[2];                   \
    } while (0)

// i -= j（192 位；同上）
#define sub_u192(i, j)                             \
    do {                                           \
        uint64_t _b_ = ((i)[0] < (j)[0]) ? 1 : 0;  \
        (i)[0] -= (j)[0];                          \
        uint64_t _b1_ = ((i)[1] < (j)[1]) ? 1 : 0; \
        (i)[1] -= (j)[1];                          \
        _b1_ += ((i)[1] < _b_) ? 1 : 0;            \
        (i)[1] -= _b_;                             \
        (i)[2] = (i)[2] - ((j)[2] + _b1_);         \
    } while (0)

/* ===================== 64bit 版本（输出 64bit） ===================== */

uint64_t log2_fixed_64(uint64_t x) {
    unsigned i = (unsigned)(x >> 57);
    uint64_t xl = x & 0x01FFFFFFFFFFFFFFull;

    /*
        v = u*2^128 = xl*2^71/(128+i)，u < 2^-7。乘子 iv = floor(2^71/(128+i))
        由 128bit 倒数表右移导出；i=0 时精确值 2^64 超出单 limb，钳位为
        2^64-1（v 误差 < xl < 2^57，相对 < 2^-64，可忽略）。xl*iv <
        2^57*2^64 = 2^121，128bit 全积恰为 v 本身（尺度已对齐）。
    */
    uint64_t v[2];
    uint64_t iv = i ? ((lg2_R128[i][1] << 7) | (lg2_R128[i][0] >> 57)) : UINT64_MAX;
    _umul64to128_(xl, iv, &v[0], &v[1]);

    // w = u^2*2^64（链乘子），wf = u^2*2^128（收尾乘子）
    uint64_t rr[4];
    _usqr128to256_(v[1], v[0], rr);
    uint64_t w = rr[3];

    /*
        64bit 输出下 B 误差预算 ~2^-59（经 u 因子放大前），w 每层衰减
        2^-14，整条链单 limb（2^63 半尺度）即可：lg2_c 顶部 limb 即
        2^63 尺度。B 为奇次项（m=1..9），A 为偶次项（m=2..10）。
    */
    uint64_t B = lg2_c[8][2];
    for (int k = 6; k >= 0; k -= 2) B = _umul64to64hi_(B, w) + lg2_c[k][2];
    uint64_t A = lg2_c[9][2];
    for (int k = 7; k >= 1; k -= 2) A = _umul64to64hi_(A, w) + lg2_c[k][2];

    // t1 = (v*B)>>64、t2 = (wf*A)>>64，均取 2^127 半尺度
    uint64_t p[2], q[2], t1[2], t2[2];
    _umul64to128_(v[1], B, &p[0], &p[1]);
    _umul64to128_(v[0], B, &q[0], &q[1]);
    t1[0] = q[1] + p[0];
    t1[1] = p[1] + (t1[0] < q[1]);
    _umul64to128_(rr[3], A, &p[0], &p[1]);
    _umul64to128_(rr[2], A, &q[0], &q[1]);
    t2[0] = q[1] + p[0];
    t2[1] = p[1] + (t2[0] < q[1]);

    // result = (L[i] + 2*(t1-t2))>>64，溢出 2^128 时钳位；L 复用 L192 高 2 limb
    sub_u128(t1, t1, t2);
    t1[0] = (t1[0] << 1) | (t1[1] >> 63);
    t1[1] <<= 1;
    add_u128(t1, t1, lg2_L192[i] + 1);
    if (t1[1] < lg2_L192[i][2]) return UINT64_MAX;
    return t1[1];
}

uint64_t exp2_fixed_64(uint64_t x) {
    unsigned i = (unsigned)(x >> 57);
    uint64_t t = x & 0x01FFFFFFFFFFFFFFull;

    // w = t^2*2^64，z = t^4*2^64
    uint64_t wf[2];
    _umul64to128_(t, t, &wf[0], &wf[1]);
    uint64_t w = wf[1];
    uint64_t z = _umul64to64hi_(w, w);

    // B = (b0+w*b1) + z*(b2+w*b3)，A = a0+w*(a1+w*a2)，单 limb 满尺度链
    uint64_t p1 = _umul64to64hi_(xp2_c[2][2], w) + xp2_c[0][2];
    uint64_t p2 = _umul64to64hi_(xp2_c[6][2], w) + xp2_c[4][2];
    uint64_t B = _umul64to64hi_(p2, z) + p1;
    uint64_t A = _umul64to64hi_(xp2_c[5][2], w) + xp2_c[3][2];
    A = _umul64to64hi_(A, w) + xp2_c[1][2];

    // S = s*2^128 = t*B + w*A，sh = round(s*2^64)
    uint64_t s0[2], s1[2];
    _umul64to128_(t, B, &s0[0], &s0[1]);
    _umul64to128_(w, A, &s1[0], &s1[1]);
    uint64_t sl = s0[0] + s1[0];
    uint64_t sh = s0[1] + s1[1] + (sl < s0[0]) + (sl >> 63);

    /*
        q = (2^(i/128)-1)*2^64：两级表 128bit 全量合成（丢弃表低位分数
        会被 T16 的大因子放大 ~1ulp，须完整求和），舍入取高 64bit。
        mm 同样舍入化，全链误差 |e| <= 2。
    */
    unsigned k = i >> 3, m = i & 7;
    uint64_t t8h = xp2_T8[m][1] + (xp2_T8[m][0] >> 63);
    uint64_t t8t[2] = {t8h, xp2_T8[m][2] + (t8h < xp2_T8[m][1])};
    uint64_t qq[2];
    umul128x128_tohi128(qq, xp2_T16[k], t8t);
    uint64_t q0 = xp2_T16[k][0] + qq[0];
    uint64_t cy = (q0 < qq[0]);
    q0 += t8t[0];
    cy += (q0 < t8t[0]);
    uint64_t q = xp2_T16[k][1] + t8t[1] + qq[1] + cy + (q0 >> 63);
    uint64_t ml, mh;
    _umul64to128_(q, sh, &ml, &mh);
    uint64_t r = q + sh + mh + (ml >> 63);
    if (r < sh) return UINT64_MAX;
    return r;
}

/* ===================== 128bit 版本（输出 128bit） ===================== */

void log2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low) {
    unsigned i = (unsigned)(high >> 57);
    uint64_t h2 = high & 0x01FFFFFFFFFFFFFFull;

    // xls = xl*2^7（128bit 对齐），v = u*2^128 = hi128(xls*R[i])
    uint64_t xls[2] = {low << 7, (h2 << 7) | (low >> 57)};
    uint64_t rr[4];
    _umul128to256_(xls[1], xls[0], lg2_R128[i][1], lg2_R128[i][0], rr);
    uint64_t v[2] = {rr[2], rr[3]};

    // wf = u^2*2^128, z = u^4*2^128, z2 = u^8*2^128 = hi128(z^2)
    _usqr128to256_(v[1], v[0], rr);
    uint64_t wf[2] = {rr[2], rr[3]};
    uint64_t zz[4];
    _usqr128to256_(wf[1], wf[0], zz);
    uint64_t z[2] = {zz[2], zz[3]};
    uint64_t z2[2];
    umul128x128_tohi128(z2, z, z);

    /*
        B(w) = (C0 + w*C1) + z*(C2 + w*C3)，A(w) 同构（偶次项）。链内
        stride-4（k -> k+4），链内乘子为 w^4 = z2，外层以 w、z 组合；
        计算按“先深后浅”顺序融合，限制同时存活的累加器个数。

        128bit 输出下 B 的误差预算 ~2^-122，链取双 limb（2^127 半尺度，
        旧版为全链 192bit）。奇次项 b_k = c_(2k+1) 存于 lg2_c[2k]（10
        项），偶次项 a_k = c_(2k+2) 存于 lg2_c[2k+1]（9 项）。
    */
    uint64_t B[2];
    {
        uint64_t c3[2] = {lg2_c[14][1], lg2_c[14][2]};  // b7
        umul128x128_tohi128(c3, c3, z2);
        add_u128(c3, c3, lg2_c[6] + 1);  // b3 + z2*b7
        umul128x128_tohi128(c3, c3, wf); // w*(b3 + z2*b7)

        uint64_t c2[2] = {lg2_c[12][1], lg2_c[12][2]};  // b6
        umul128x128_tohi128(c2, c2, z2);
        add_u128(c2, c2, lg2_c[4] + 1);  // b2 + z2*b6
        add_u128(c2, c2, c3);
        umul128x128_tohi128(c2, c2, z);  // z*(C2 + w*C3)

        uint64_t c1[2] = {lg2_c[18][1], lg2_c[18][2]};  // b9
        umul128x128_tohi128(c1, c1, z2);
        add_u128(c1, c1, lg2_c[10] + 1);  // b5 + z2*b9
        umul128x128_tohi128(c1, c1, z2);
        add_u128(c1, c1, lg2_c[2] + 1);  // b1 + ...
        umul128x128_tohi128(c1, c1, wf); // w*C1

        uint64_t c0[2] = {lg2_c[16][1], lg2_c[16][2]};  // b8
        umul128x128_tohi128(c0, c0, z2);
        add_u128(c0, c0, lg2_c[8] + 1);  // b4 + z2*b8
        umul128x128_tohi128(c0, c0, z2);
        add_u128(c0, c0, lg2_c[0] + 1);  // b0 + ...

        add_u128(c1, c1, c0);  // C0 + w*C1
        add_u128(B, c1, c2);   // B
    }

    uint64_t A[2];
    {
        uint64_t d3[2] = {lg2_c[15][1], lg2_c[15][2]};  // a7
        umul128x128_tohi128(d3, d3, z2);
        add_u128(d3, d3, lg2_c[7] + 1);  // a3 + z2*a7
        umul128x128_tohi128(d3, d3, wf); // w*(...)

        uint64_t d2[2] = {lg2_c[13][1], lg2_c[13][2]};  // a6
        umul128x128_tohi128(d2, d2, z2);
        add_u128(d2, d2, lg2_c[5] + 1);  // a2 + z2*a6
        add_u128(d2, d2, d3);
        umul128x128_tohi128(d2, d2, z);  // z*(D2 + w*D3)

        uint64_t d1[2] = {lg2_c[11][1], lg2_c[11][2]};  // a5
        umul128x128_tohi128(d1, d1, z2);
        add_u128(d1, d1, lg2_c[3] + 1);  // a1 + z2*a5
        umul128x128_tohi128(d1, d1, wf); // w*D1

        uint64_t d0[2] = {lg2_c[17][1], lg2_c[17][2]};  // a8
        umul128x128_tohi128(d0, d0, z2);
        add_u128(d0, d0, lg2_c[9] + 1);  // a4 + z2*a8
        umul128x128_tohi128(d0, d0, z2);
        add_u128(d0, d0, lg2_c[1] + 1);  // a0 + ...

        add_u128(d1, d1, d0);  // D0 + w*D1
        add_u128(A, d1, d2);   // A
    }

    // P = 2*(u*B - wf*A)，t1/t2 取 256bit 积的高 192bit（2^191 半尺度）
    uint64_t t1[3], t2[3];
    _umul128to256_(v[1], v[0], B[1], B[0], rr);
    t1[0] = rr[1];
    t1[1] = rr[2];
    t1[2] = rr[3];
    _umul128to256_(wf[1], wf[0], A[1], A[0], zz);
    t2[0] = zz[1];
    t2[1] = zz[2];
    t2[2] = zz[3];
    sub_u192(t1, t2);
    t1[2] = (t1[2] << 1) | (t1[1] >> 63);
    t1[1] = (t1[1] << 1) | (t1[0] >> 63);
    t1[0] <<= 1;

    // result192 = L192[i] + P，加 2^63 使 >>64 截断舍入化，输出高 128bit
    add_u192(t1, lg2_L192[i]);
    if (t1[2] < lg2_L192[i][2]) {
        dst[0] = dst[1] = UINT64_MAX;
        return;
    }
    t1[0] += 0x8000000000000000ull;
    if (t1[0] < 0x8000000000000000ull) {
        if (++t1[1] == 0 && ++t1[2] == 0) {
            dst[0] = dst[1] = UINT64_MAX;
            return;
        }
    }
    dst[0] = t1[1];
    dst[1] = t1[2];
}

void exp2_fixed_128(uint64_t* dst, uint64_t high, uint64_t low) {
    unsigned i = (unsigned)(high >> 57);
    uint64_t W[2] = {low, high & 0x01FFFFFFFFFFFFFFull};  // W = t*2^128（精确）

    // wf = t^2*2^128, z = t^4*2^128, z2 = t^8*2^128 = hi128(z^2)
    uint64_t rr[4], zz[4];
    _usqr128to256_(W[1], W[0], rr);
    uint64_t wf[2] = {rr[2], rr[3]};
    _usqr128to256_(wf[1], wf[0], zz);
    uint64_t z[2] = {zz[2], zz[3]};
    uint64_t z2[2];
    umul128x128_tohi128(z2, z, z);

    /*
        B(w) = (C0 + w*C1) + z*(C2 + w*C3)，链内 stride-4 故链内乘子
        w^4 = z2。链取双 limb（2^128 满尺度，xp2 系数均 < 1）。奇次项
        b_k = c_(2k+1) 存于 xp2_c[2k]（7 项，b3 为单系数链）；偶次项
        a_k = c_(2k+2)（6 项）。
    */
    uint64_t C0[2] = {xp2_c[8][1], xp2_c[8][2]};
    umul128x128_tohi128(C0, C0, z2);
    add_u128(C0, C0, xp2_c[0] + 1);

    uint64_t C1[2] = {xp2_c[10][1], xp2_c[10][2]};
    umul128x128_tohi128(C1, C1, z2);
    add_u128(C1, C1, xp2_c[2] + 1);

    uint64_t C2[2] = {xp2_c[12][1], xp2_c[12][2]};
    umul128x128_tohi128(C2, C2, z2);
    add_u128(C2, C2, xp2_c[4] + 1);

    umul128x128_tohi128(C1, C1, wf);
    add_u128(C1, C1, C0);             // C1 <- C0 + w*C1
    uint64_t C3[2] = {xp2_c[6][1], xp2_c[6][2]};  // b3
    umul128x128_tohi128(C3, C3, wf);
    add_u128(C3, C3, C2);
    umul128x128_tohi128(C3, C3, z);
    add_u128(C1, C1, C3);  // C1 <- B

    uint64_t D0[2] = {xp2_c[9][1], xp2_c[9][2]};
    umul128x128_tohi128(D0, D0, z2);
    add_u128(D0, D0, xp2_c[1] + 1);

    uint64_t D1[2] = {xp2_c[11][1], xp2_c[11][2]};
    umul128x128_tohi128(D1, D1, z2);
    add_u128(D1, D1, xp2_c[3] + 1);

    umul128x128_tohi128(D1, D1, wf);
    add_u128(D1, D1, D0);             // D1 <- D0 + w*D1
    uint64_t D2[2] = {xp2_c[5][1], xp2_c[5][2]};  // a2
    uint64_t D3[2] = {xp2_c[7][1], xp2_c[7][2]};  // a3
    umul128x128_tohi128(D3, D3, wf);
    add_u128(D3, D3, D2);
    umul128x128_tohi128(D3, D3, z);
    add_u128(D1, D1, D3);  // D1 <- A

    // S = s*2^192 = t*B + w*A（取 256bit 积高 192bit），sh = s*2^128
    uint64_t S[3], u2[3];
    _umul128to256_(W[1], W[0], C1[1], C1[0], rr);
    S[0] = rr[1];
    S[1] = rr[2];
    S[2] = rr[3];
    _umul128to256_(wf[1], wf[0], D1[1], D1[0], zz);
    u2[0] = zz[1];
    u2[1] = zz[2];
    u2[2] = zz[3];
    add_u192(S, u2);
    uint64_t sh[2];
    sh[0] = S[1] + (S[0] >> 63);
    sh[1] = S[2] + (sh[0] < S[1]);

    // q = (2^(i/128)-1)*2^128 = T16 + T8高2limb(舍入) + hi128(T16*T8)
    unsigned k = i >> 3, m = i & 7;
    uint64_t t8h = xp2_T8[m][1] + (xp2_T8[m][0] >> 63);
    uint64_t t8t[2] = {t8h, xp2_T8[m][2] + (t8h < xp2_T8[m][1])};
    uint64_t q[2], m2[2];
    _umul128to256_(xp2_T16[k][1], xp2_T16[k][0], t8t[1], t8t[0], rr);
    m2[0] = rr[2] + (rr[1] >> 63);
    m2[1] = rr[3] + (m2[0] < rr[2]);
    q[0] = xp2_T16[k][0];
    q[1] = xp2_T16[k][1];
    add_u128(q, q, t8t);
    add_u128(q, q, m2);

    /*
        result = q + sh + round128(q*sh)：q/sh 的构造与 m 的 hi128 均已
        舍入化，加 1 中心化后全链 |e| <= 2。
    */
    _umul128to256_(q[1], q[0], sh[1], sh[0], rr);
    m2[0] = rr[2] + (rr[1] >> 63);
    m2[1] = rr[3] + (m2[0] < rr[2]);
    uint64_t r0 = q[0], r1 = q[1], cy;
    r0 += sh[0], cy = (r0 < sh[0]);
    r1 += cy, cy = (r1 < cy);
    r1 += sh[1], cy += (r1 < sh[1]);
    if (cy) {
        dst[0] = dst[1] = UINT64_MAX;
        return;
    }
    r0 += m2[0], cy = (r0 < m2[0]);
    r1 += cy, cy = (r1 < cy);
    r1 += m2[1], cy += (r1 < m2[1]);
    dst[0] = r0;
    dst[1] = r1;
}
