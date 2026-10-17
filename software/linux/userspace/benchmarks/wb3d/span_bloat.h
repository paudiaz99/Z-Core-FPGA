/* SPDX-License-Identifier: MIT
 * Copyright (c) 2025–2026 Pau Díaz Cuesta
 */
#ifndef WB3D_SPAN_BLOAT_H
#define WB3D_SPAN_BLOAT_H

#define SPANFN(n)                                                              \
    static void __attribute__((noinline, aligned(256)))                        \
    span_##n(uint8_t *cb, uint32_t *zb, int idx, int len,                      \
             int32_t z, int32_t dzdx, uint8_t col)                             \
    {                                                                          \
        __asm__ volatile("" :: "i"(n));                                        \
        span_body(cb, zb, idx, len, z, dzdx, col);                             \
    }

SPANFN(0)
SPANFN(1)
SPANFN(2)
SPANFN(3)
SPANFN(4)
SPANFN(5)
SPANFN(6)
SPANFN(7)
SPANFN(8)
SPANFN(9)
SPANFN(10)
SPANFN(11)
SPANFN(12)
SPANFN(13)
SPANFN(14)
SPANFN(15)
SPANFN(16)
SPANFN(17)
SPANFN(18)
SPANFN(19)
SPANFN(20)
SPANFN(21)
SPANFN(22)
SPANFN(23)
SPANFN(24)
SPANFN(25)
SPANFN(26)
SPANFN(27)
SPANFN(28)
SPANFN(29)
SPANFN(30)
SPANFN(31)
SPANFN(32)
SPANFN(33)
SPANFN(34)
SPANFN(35)
SPANFN(36)
SPANFN(37)
SPANFN(38)
SPANFN(39)
SPANFN(40)
SPANFN(41)
SPANFN(42)
SPANFN(43)
SPANFN(44)
SPANFN(45)
SPANFN(46)
SPANFN(47)
SPANFN(48)
SPANFN(49)
SPANFN(50)
SPANFN(51)
SPANFN(52)
SPANFN(53)
SPANFN(54)
SPANFN(55)
SPANFN(56)
SPANFN(57)
SPANFN(58)
SPANFN(59)
SPANFN(60)
SPANFN(61)
SPANFN(62)
SPANFN(63)
SPANFN(64)
SPANFN(65)
SPANFN(66)
SPANFN(67)
SPANFN(68)
SPANFN(69)
SPANFN(70)
SPANFN(71)
SPANFN(72)
SPANFN(73)
SPANFN(74)
SPANFN(75)
SPANFN(76)
SPANFN(77)
SPANFN(78)
SPANFN(79)
SPANFN(80)
SPANFN(81)
SPANFN(82)
SPANFN(83)
SPANFN(84)
SPANFN(85)
SPANFN(86)
SPANFN(87)
SPANFN(88)
SPANFN(89)
SPANFN(90)
SPANFN(91)
SPANFN(92)
SPANFN(93)
SPANFN(94)
SPANFN(95)
SPANFN(96)
SPANFN(97)
SPANFN(98)
SPANFN(99)
SPANFN(100)
SPANFN(101)
SPANFN(102)
SPANFN(103)
SPANFN(104)
SPANFN(105)
SPANFN(106)
SPANFN(107)
SPANFN(108)
SPANFN(109)
SPANFN(110)
SPANFN(111)
SPANFN(112)
SPANFN(113)
SPANFN(114)
SPANFN(115)
SPANFN(116)
SPANFN(117)
SPANFN(118)
SPANFN(119)
SPANFN(120)
SPANFN(121)
SPANFN(122)
SPANFN(123)
SPANFN(124)
SPANFN(125)
SPANFN(126)
SPANFN(127)
SPANFN(128)
SPANFN(129)
SPANFN(130)
SPANFN(131)
SPANFN(132)
SPANFN(133)
SPANFN(134)
SPANFN(135)
SPANFN(136)
SPANFN(137)
SPANFN(138)
SPANFN(139)
SPANFN(140)
SPANFN(141)
SPANFN(142)
SPANFN(143)
SPANFN(144)
SPANFN(145)
SPANFN(146)
SPANFN(147)
SPANFN(148)
SPANFN(149)
SPANFN(150)
SPANFN(151)
SPANFN(152)
SPANFN(153)
SPANFN(154)
SPANFN(155)
SPANFN(156)
SPANFN(157)
SPANFN(158)
SPANFN(159)
SPANFN(160)
SPANFN(161)
SPANFN(162)
SPANFN(163)
SPANFN(164)
SPANFN(165)
SPANFN(166)
SPANFN(167)
SPANFN(168)
SPANFN(169)
SPANFN(170)
SPANFN(171)
SPANFN(172)
SPANFN(173)
SPANFN(174)
SPANFN(175)
SPANFN(176)
SPANFN(177)
SPANFN(178)
SPANFN(179)
SPANFN(180)
SPANFN(181)
SPANFN(182)
SPANFN(183)
SPANFN(184)
SPANFN(185)
SPANFN(186)
SPANFN(187)
SPANFN(188)
SPANFN(189)
SPANFN(190)
SPANFN(191)
SPANFN(192)
SPANFN(193)
SPANFN(194)
SPANFN(195)
SPANFN(196)
SPANFN(197)
SPANFN(198)
SPANFN(199)
SPANFN(200)
SPANFN(201)
SPANFN(202)
SPANFN(203)
SPANFN(204)
SPANFN(205)
SPANFN(206)
SPANFN(207)
SPANFN(208)
SPANFN(209)
SPANFN(210)
SPANFN(211)
SPANFN(212)
SPANFN(213)
SPANFN(214)
SPANFN(215)
SPANFN(216)
SPANFN(217)
SPANFN(218)
SPANFN(219)
SPANFN(220)
SPANFN(221)
SPANFN(222)
SPANFN(223)
SPANFN(224)
SPANFN(225)
SPANFN(226)
SPANFN(227)
SPANFN(228)
SPANFN(229)
SPANFN(230)
SPANFN(231)
SPANFN(232)
SPANFN(233)
SPANFN(234)
SPANFN(235)
SPANFN(236)
SPANFN(237)
SPANFN(238)
SPANFN(239)
SPANFN(240)
SPANFN(241)
SPANFN(242)
SPANFN(243)
SPANFN(244)
SPANFN(245)
SPANFN(246)
SPANFN(247)
SPANFN(248)
SPANFN(249)
SPANFN(250)
SPANFN(251)
SPANFN(252)
SPANFN(253)
SPANFN(254)
SPANFN(255)

#define SPAN_TBL_N 256

static span_fn const span_tbl[SPAN_TBL_N] = {
    span_0, span_1, span_2, span_3,
    span_4, span_5, span_6, span_7,
    span_8, span_9, span_10, span_11,
    span_12, span_13, span_14, span_15,
    span_16, span_17, span_18, span_19,
    span_20, span_21, span_22, span_23,
    span_24, span_25, span_26, span_27,
    span_28, span_29, span_30, span_31,
    span_32, span_33, span_34, span_35,
    span_36, span_37, span_38, span_39,
    span_40, span_41, span_42, span_43,
    span_44, span_45, span_46, span_47,
    span_48, span_49, span_50, span_51,
    span_52, span_53, span_54, span_55,
    span_56, span_57, span_58, span_59,
    span_60, span_61, span_62, span_63,
    span_64, span_65, span_66, span_67,
    span_68, span_69, span_70, span_71,
    span_72, span_73, span_74, span_75,
    span_76, span_77, span_78, span_79,
    span_80, span_81, span_82, span_83,
    span_84, span_85, span_86, span_87,
    span_88, span_89, span_90, span_91,
    span_92, span_93, span_94, span_95,
    span_96, span_97, span_98, span_99,
    span_100, span_101, span_102, span_103,
    span_104, span_105, span_106, span_107,
    span_108, span_109, span_110, span_111,
    span_112, span_113, span_114, span_115,
    span_116, span_117, span_118, span_119,
    span_120, span_121, span_122, span_123,
    span_124, span_125, span_126, span_127,
    span_128, span_129, span_130, span_131,
    span_132, span_133, span_134, span_135,
    span_136, span_137, span_138, span_139,
    span_140, span_141, span_142, span_143,
    span_144, span_145, span_146, span_147,
    span_148, span_149, span_150, span_151,
    span_152, span_153, span_154, span_155,
    span_156, span_157, span_158, span_159,
    span_160, span_161, span_162, span_163,
    span_164, span_165, span_166, span_167,
    span_168, span_169, span_170, span_171,
    span_172, span_173, span_174, span_175,
    span_176, span_177, span_178, span_179,
    span_180, span_181, span_182, span_183,
    span_184, span_185, span_186, span_187,
    span_188, span_189, span_190, span_191,
    span_192, span_193, span_194, span_195,
    span_196, span_197, span_198, span_199,
    span_200, span_201, span_202, span_203,
    span_204, span_205, span_206, span_207,
    span_208, span_209, span_210, span_211,
    span_212, span_213, span_214, span_215,
    span_216, span_217, span_218, span_219,
    span_220, span_221, span_222, span_223,
    span_224, span_225, span_226, span_227,
    span_228, span_229, span_230, span_231,
    span_232, span_233, span_234, span_235,
    span_236, span_237, span_238, span_239,
    span_240, span_241, span_242, span_243,
    span_244, span_245, span_246, span_247,
    span_248, span_249, span_250, span_251,
    span_252, span_253, span_254, span_255,
};

#endif
