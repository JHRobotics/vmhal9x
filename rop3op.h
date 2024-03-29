/******************************************************************************
 * Copyright (c) 2023 Jaroslav Hensl                                          *
 ******************************************************************************/

/*
 * https://learn.microsoft.com/en-us/windows/win32/gdi/ternary-raster-operations
*/

#define ROP3_00(_D, _P, _S) 0
#define ROP3_01(_D, _P, _S) (~((_D) | ((_P) | (_S))))
#define ROP3_02(_D, _P, _S) ((_D) & (~((_P) | (_S))))
#define ROP3_03(_D, _P, _S) (~((_P) | (_S)))
#define ROP3_04(_D, _P, _S) ((_S) & (~((_D) | (_P))))
#define ROP3_05(_D, _P, _S) (~((_D) | (_P)))
#define ROP3_06(_D, _P, _S) (~((_P) | (~((_D) ^ (_S)))))
#define ROP3_07(_D, _P, _S) (~((_P) | ((_D) & (_S))))
#define ROP3_08(_D, _P, _S) ((_S) & ((_D) & (~(_P))))
#define ROP3_09(_D, _P, _S) (~((_P) | ((_D) ^ (_S))))
#define ROP3_0A(_D, _P, _S) ((_D) & (~(_P)))
#define ROP3_0B(_D, _P, _S) (~((_P) | ((_S) & (~(_D)))))
#define ROP3_0C(_D, _P, _S) ((_S) & (~(_P)))
#define ROP3_0D(_D, _P, _S) (~((_P) | ((_D) & (~(_S)))))
#define ROP3_0E(_D, _P, _S) (~((_P) | (~((_D) | (_S)))))
#define ROP3_0F(_D, _P, _S) (~(_P))
#define ROP3_10(_D, _P, _S) ((_P) & (~((_D) | (_S))))
#define ROP3_11(_D, _P, _S) (~((_D) | (_S)))
#define ROP3_12(_D, _P, _S) (~((_S) | (~((_D) ^ (_P)))))
#define ROP3_13(_D, _P, _S) (~((_S) | ((_D) & (_P))))
#define ROP3_14(_D, _P, _S) (~((_D) | (~((_P) ^ (_S)))))
#define ROP3_15(_D, _P, _S) (~((_D) | ((_P) & (_S))))
#define ROP3_16(_D, _P, _S) ((_P) ^ ((_S) ^ ((_D) & (~((_P) & (_S))))))
#define ROP3_17(_D, _P, _S) (~((_S) ^ (((_S) ^ (_P)) & ((_D) ^ (_S)))))
#define ROP3_18(_D, _P, _S) (((_S) ^ (_P)) & ((_P) ^ (_D)))
#define ROP3_19(_D, _P, _S) (~((_S) ^ ((_D) & (~((_P) & (_S))))))
#define ROP3_1A(_D, _P, _S) ((_P) ^ ((_D) | ((_S) & (_P))))
#define ROP3_1B(_D, _P, _S) (~((_S) ^ ((_D) & ((_P) ^ (_S)))))
#define ROP3_1C(_D, _P, _S) ((_P) ^ ((_S) | ((_D) & (_P))))
#define ROP3_1D(_D, _P, _S) (~((_D) ^ ((_S) & ((_P) ^ (_D)))))
#define ROP3_1E(_D, _P, _S) ((_P) ^ ((_D) | (_S)))
#define ROP3_1F(_D, _P, _S) (~((_P) & ((_D) | (_S))))
#define ROP3_20(_D, _P, _S) ((_D) & ((_P) & (~(_S))))
#define ROP3_21(_D, _P, _S) (~((_S) | ((_D) ^ (_P))))
#define ROP3_22(_D, _P, _S) ((_D) & (~(_S)))
#define ROP3_23(_D, _P, _S) (~((_S) | ((_P) & (~(_D)))))
#define ROP3_24(_D, _P, _S) (((_S) ^ (_P)) & ((_D) ^ (_S)))
#define ROP3_25(_D, _P, _S) (~((_P) ^ ((_D) & (~((_S) & (_P))))))
#define ROP3_26(_D, _P, _S) ((_S) ^ ((_D) | ((_P) & (_S))))
#define ROP3_27(_D, _P, _S) ((_S) ^ ((_D) | (~((_P) ^ (_S)))))
#define ROP3_28(_D, _P, _S) ((_D) & ((_P) ^ (_S)))
#define ROP3_29(_D, _P, _S) (~((_P) ^ ((_S) ^ ((_D) | ((_P) & (_S))))))
#define ROP3_2A(_D, _P, _S) ((_D) & (~((_P) & (_S))))
#define ROP3_2B(_D, _P, _S) (~((_S) ^ (((_S) ^ (_P)) & ((_P) ^ (_D)))))
#define ROP3_2C(_D, _P, _S) ((_S) ^ ((_P) & ((_D) | (_S))))
#define ROP3_2D(_D, _P, _S) ((_P) ^ ((_S) | (~(_D))))
#define ROP3_2E(_D, _P, _S) ((_P) ^ ((_S) | ((_D) ^ (_P))))
#define ROP3_2F(_D, _P, _S) (~((_P) & ((_S) | (~(_D)))))
#define ROP3_30(_D, _P, _S) ((_P) & (~(_S)))
#define ROP3_31(_D, _P, _S) (~((_S) | ((_D) & (~(_P)))))
#define ROP3_32(_D, _P, _S) ((_S) ^ ((_D) | ((_P) | (_S))))
#define ROP3_33(_D, _P, _S) (~(_S))
#define ROP3_34(_D, _P, _S) ((_S) ^ ((_P) | ((_D) & (_S))))
#define ROP3_35(_D, _P, _S) ((_S) ^ ((_P) | (~((_D) ^ (_S)))))
#define ROP3_36(_D, _P, _S) ((_S) ^ ((_D) | (_P)))
#define ROP3_37(_D, _P, _S) (~((_S) & ((_D) | (_P))))
#define ROP3_38(_D, _P, _S) ((_P) ^ ((_S) & ((_D) | (_P))))
#define ROP3_39(_D, _P, _S) ((_S) ^ ((_P) | (~(_D))))
#define ROP3_3A(_D, _P, _S) ((_S) ^ ((_P) | ((_D) ^ (_S))))
#define ROP3_3B(_D, _P, _S) (~((_S) & ((_P) | (~(_D)))))
#define ROP3_3C(_D, _P, _S) ((_P) ^ (_S))
#define ROP3_3D(_D, _P, _S) ((_S) ^ ((_P) | (~((_D) | (_S)))))
#define ROP3_3E(_D, _P, _S) ((_S) ^ ((_P) | ((_D) & (~(_S)))))
#define ROP3_3F(_D, _P, _S) (~((_P) & (_S)))
#define ROP3_40(_D, _P, _S) ((_P) & ((_S) & (~(_D))))
#define ROP3_41(_D, _P, _S) (~((_D) | ((_P) ^ (_S))))
#define ROP3_42(_D, _P, _S) (((_S) ^ (_D)) & ((_P) ^ (_D)))
#define ROP3_43(_D, _P, _S) (~((_S) ^ ((_P) & (~((_D) & (_S))))))
#define ROP3_44(_D, _P, _S) ((_S) & (~(_D)))
#define ROP3_45(_D, _P, _S) (~((_D) | ((_P) & (~(_S)))))
#define ROP3_46(_D, _P, _S) ((_D) ^ ((_S) | ((_P) & (_D))))
#define ROP3_47(_D, _P, _S) (~((_P) ^ ((_S) & ((_D) ^ (_P)))))
#define ROP3_48(_D, _P, _S) ((_S) & ((_D) ^ (_P)))
#define ROP3_49(_D, _P, _S) (~((_P) ^ ((_D) ^ ((_S) | ((_P) & (_D))))))
#define ROP3_4A(_D, _P, _S) ((_D) ^ ((_P) & ((_S) | (_D))))
#define ROP3_4B(_D, _P, _S) ((_P) ^ ((_D) | (~(_S))))
#define ROP3_4C(_D, _P, _S) ((_S) & (~((_D) & (_P))))
#define ROP3_4D(_D, _P, _S) (~((_S) ^ (((_S) ^ (_P)) | ((_D) ^ (_S)))))
#define ROP3_4E(_D, _P, _S) ((_P) ^ ((_D) | ((_S) ^ (_P))))
#define ROP3_4F(_D, _P, _S) (~((_P) & ((_D) | (~(_S)))))
#define ROP3_50(_D, _P, _S) ((_P) & (~(_D)))
#define ROP3_51(_D, _P, _S) (~((_D) | ((_S) & (~(_P)))))
#define ROP3_52(_D, _P, _S) ((_D) ^ ((_P) | ((_S) & (_D))))
#define ROP3_53(_D, _P, _S) (~((_S) ^ ((_P) & ((_D) ^ (_S)))))
#define ROP3_54(_D, _P, _S) (~((_D) | (~((_P) | (_S)))))
#define ROP3_55(_D, _P, _S) (~(_D))
#define ROP3_56(_D, _P, _S) ((_D) ^ ((_P) | (_S)))
#define ROP3_57(_D, _P, _S) (~((_D) & ((_P) | (_S))))
#define ROP3_58(_D, _P, _S) ((_P) ^ ((_D) & ((_S) | (_P))))
#define ROP3_59(_D, _P, _S) ((_D) ^ ((_P) | (~(_S))))
#define ROP3_5A(_D, _P, _S) ((_D) ^ (_P))
#define ROP3_5B(_D, _P, _S) ((_D) ^ ((_P) | (~((_S) | (_D)))))
#define ROP3_5C(_D, _P, _S) ((_D) ^ ((_P) | ((_S) ^ (_D))))
#define ROP3_5D(_D, _P, _S) (~((_D) & ((_P) | (~(_S)))))
#define ROP3_5E(_D, _P, _S) ((_D) ^ ((_P) | ((_S) & (~(_D)))))
#define ROP3_5F(_D, _P, _S) (~((_D) & (_P)))
#define ROP3_60(_D, _P, _S) ((_P) & ((_D) ^ (_S)))
#define ROP3_61(_D, _P, _S) (~((_D) ^ ((_S) ^ ((_P) | ((_D) & (_S))))))
#define ROP3_62(_D, _P, _S) ((_D) ^ ((_S) & ((_P) | (_D))))
#define ROP3_63(_D, _P, _S) ((_S) ^ ((_D) | (~(_P))))
#define ROP3_64(_D, _P, _S) ((_S) ^ ((_D) & ((_P) | (_S))))
#define ROP3_65(_D, _P, _S) ((_D) ^ ((_S) | (~(_P))))
#define ROP3_66(_D, _P, _S) ((_D) ^ (_S))
#define ROP3_67(_D, _P, _S) ((_S) ^ ((_D) | (~((_P) | (_S)))))
#define ROP3_68(_D, _P, _S) (~((_D) ^ ((_S) ^ ((_P) | (~((_D) | (_S)))))))
#define ROP3_69(_D, _P, _S) (~((_P) ^ ((_D) ^ (_S))))
#define ROP3_6A(_D, _P, _S) ((_D) ^ ((_P) & (_S)))
#define ROP3_6B(_D, _P, _S) (~((_P) ^ ((_S) ^ ((_D) & ((_P) | (_S))))))
#define ROP3_6C(_D, _P, _S) ((_S) ^ ((_D) & (_P)))
#define ROP3_6D(_D, _P, _S) (~((_P) ^ ((_D) ^ ((_S) & ((_P) | (_D))))))
#define ROP3_6E(_D, _P, _S) ((_S) ^ ((_D) & ((_P) | (~(_S)))))
#define ROP3_6F(_D, _P, _S) (~((_P) & (~((_D) ^ (_S)))))
#define ROP3_70(_D, _P, _S) ((_P) & (~((_D) & (_S))))
#define ROP3_71(_D, _P, _S) (~((_S) ^ (((_S) ^ (_D)) & ((_P) ^ (_D)))))
#define ROP3_72(_D, _P, _S) ((_S) ^ ((_D) | ((_P) ^ (_S))))
#define ROP3_73(_D, _P, _S) (~((_S) & ((_D) | (~(_P)))))
#define ROP3_74(_D, _P, _S) ((_D) ^ ((_S) | ((_P) ^ (_D))))
#define ROP3_75(_D, _P, _S) (~((_D) & ((_S) | (~(_P)))))
#define ROP3_76(_D, _P, _S) ((_S) ^ ((_D) | ((_P) & (~(_S)))))
#define ROP3_77(_D, _P, _S) (~((_D) & (_S)))
#define ROP3_78(_D, _P, _S) ((_P) ^ ((_D) & (_S)))
#define ROP3_79(_D, _P, _S) (~((_D) ^ ((_S) ^ ((_P) & ((_D) | (_S))))))
#define ROP3_7A(_D, _P, _S) ((_D) ^ ((_P) & ((_S) | (~(_D)))))
#define ROP3_7B(_D, _P, _S) (~((_S) & (~((_D) ^ (_P)))))
#define ROP3_7C(_D, _P, _S) ((_S) ^ ((_P) & ((_D) | (~(_S)))))
#define ROP3_7D(_D, _P, _S) (~((_D) & (~((_P) ^ (_S)))))
#define ROP3_7E(_D, _P, _S) (((_S) ^ (_P)) | ((_D) ^ (_S)))
#define ROP3_7F(_D, _P, _S) (~((_D) & ((_P) & (_S))))
#define ROP3_80(_D, _P, _S) ((_D) & ((_P) & (_S)))
#define ROP3_81(_D, _P, _S) (~(((_S) ^ (_P)) | ((_D) ^ (_S))))
#define ROP3_82(_D, _P, _S) ((_D) & (~((_P) ^ (_S))))
#define ROP3_83(_D, _P, _S) (~((_S) ^ ((_P) & ((_D) | (~(_S))))))
#define ROP3_84(_D, _P, _S) ((_S) & (~((_D) ^ (_P))))
#define ROP3_85(_D, _P, _S) (~((_P) ^ ((_D) & ((_S) | (~(_P))))))
#define ROP3_86(_D, _P, _S) ((_D) ^ ((_S) ^ ((_P) & ((_D) | (_S)))))
#define ROP3_87(_D, _P, _S) (~((_P) ^ ((_D) & (_S))))
#define ROP3_88(_D, _P, _S) ((_D) & (_S))
#define ROP3_89(_D, _P, _S) (~((_S) ^ ((_D) | ((_P) & (~(_S))))))
#define ROP3_8A(_D, _P, _S) ((_D) & ((_S) | (~(_P))))
#define ROP3_8B(_D, _P, _S) (~((_D) ^ ((_S) | ((_P) ^ (_D)))))
#define ROP3_8C(_D, _P, _S) ((_S) & ((_D) | (~(_P))))
#define ROP3_8D(_D, _P, _S) (~((_S) ^ ((_D) | ((_P) ^ (_S)))))
#define ROP3_8E(_D, _P, _S) ((_S) ^ (((_S) ^ (_D)) & ((_P) ^ (_D))))
#define ROP3_8F(_D, _P, _S) (~((_P) & (~((_D) & (_S)))))
#define ROP3_90(_D, _P, _S) ((_P) & (~((_D) ^ (_S))))
#define ROP3_91(_D, _P, _S) (~((_S) ^ ((_D) & ((_P) | (~(_S))))))
#define ROP3_92(_D, _P, _S) ((_D) ^ ((_P) ^ ((_S) & ((_D) | (_P)))))
#define ROP3_93(_D, _P, _S) (~((_S) ^ ((_P) & (_D))))
#define ROP3_94(_D, _P, _S) ((_P) ^ ((_S) ^ ((_D) & ((_P) | (_S)))))
#define ROP3_95(_D, _P, _S) (~((_D) ^ ((_P) & (_S))))
#define ROP3_96(_D, _P, _S) ((_D) ^ ((_P) ^ (_S)))
#define ROP3_97(_D, _P, _S) ((_P) ^ ((_S) ^ ((_D) | (~((_P) | (_S))))))
#define ROP3_98(_D, _P, _S) (~((_S) ^ ((_D) | (~((_P) | (_S))))))
#define ROP3_99(_D, _P, _S) (~((_D) ^ (_S)))
#define ROP3_9A(_D, _P, _S) ((_D) ^ ((_P) & (~(_S))))
#define ROP3_9B(_D, _P, _S) (~((_S) ^ ((_D) & ((_P) | (_S)))))
#define ROP3_9C(_D, _P, _S) ((_S) ^ ((_P) & (~(_D))))
#define ROP3_9D(_D, _P, _S) (~((_D) ^ ((_S) & ((_P) | (_D)))))
#define ROP3_9E(_D, _P, _S) ((_D) ^ ((_S) ^ ((_P) | ((_D) & (_S)))))
#define ROP3_9F(_D, _P, _S) (~((_P) & ((_D) ^ (_S))))
#define ROP3_A0(_D, _P, _S) ((_D) & (_P))
#define ROP3_A1(_D, _P, _S) (~((_P) ^ ((_D) | ((_S) & (~(_P))))))
#define ROP3_A2(_D, _P, _S) ((_D) & ((_P) | (~(_S))))
#define ROP3_A3(_D, _P, _S) (~((_D) ^ ((_P) | ((_S) ^ (_D)))))
#define ROP3_A4(_D, _P, _S) (~((_P) ^ ((_D) | (~((_S) | (_P))))))
#define ROP3_A5(_D, _P, _S) (~((_P) ^ (_D)))
#define ROP3_A6(_D, _P, _S) ((_D) ^ ((_S) & (~(_P))))
#define ROP3_A7(_D, _P, _S) (~((_P) ^ ((_D) & ((_S) | (_P)))))
#define ROP3_A8(_D, _P, _S) ((_D) & ((_P) | (_S)))
#define ROP3_A9(_D, _P, _S) (~((_D) ^ ((_P) | (_S))))
#define ROP3_AA(_D, _P, _S) _D
#define ROP3_AB(_D, _P, _S) ((_D) | (~((_P) | (_S))))
#define ROP3_AC(_D, _P, _S) ((_S) ^ ((_P) & ((_D) ^ (_S))))
#define ROP3_AD(_D, _P, _S) (~((_D) ^ ((_P) | ((_S) & (_D)))))
#define ROP3_AE(_D, _P, _S) ((_D) | ((_S) & (~(_P))))
#define ROP3_AF(_D, _P, _S) ((_D) | (~(_P)))
#define ROP3_B0(_D, _P, _S) ((_P) & ((_D) | (~(_S))))
#define ROP3_B1(_D, _P, _S) (~((_P) ^ ((_D) | ((_S) ^ (_P)))))
#define ROP3_B2(_D, _P, _S) ((_S) ^ (((_S) ^ (_P)) | ((_D) ^ (_S))))
#define ROP3_B3(_D, _P, _S) (~((_S) & (~((_D) & (_P)))))
#define ROP3_B4(_D, _P, _S) ((_P) ^ ((_S) & (~(_D))))
#define ROP3_B5(_D, _P, _S) (~((_D) ^ ((_P) & ((_S) | (_D)))))
#define ROP3_B6(_D, _P, _S) ((_D) ^ ((_P) ^ ((_S) | ((_D) & (_P)))))
#define ROP3_B7(_D, _P, _S) (~((_S) & ((_D) ^ (_P))))
#define ROP3_B8(_D, _P, _S) ((_P) ^ ((_S) & ((_D) ^ (_P))))
#define ROP3_B9(_D, _P, _S) (~((_D) ^ ((_S) | ((_P) & (_D)))))
#define ROP3_BA(_D, _P, _S) ((_D) | ((_P) & (~(_S))))
#define ROP3_BB(_D, _P, _S) ((_D) | (~(_S)))
#define ROP3_BC(_D, _P, _S) ((_S) ^ ((_P) & (~((_D) & (_S)))))
#define ROP3_BD(_D, _P, _S) (~(((_S) ^ (_D)) & ((_P) ^ (_D))))
#define ROP3_BE(_D, _P, _S) ((_D) | ((_P) ^ (_S)))
#define ROP3_BF(_D, _P, _S) ((_D) | (~((_P) & (_S))))
#define ROP3_C0(_D, _P, _S) ((_P) & (_S))
#define ROP3_C1(_D, _P, _S) (~((_S) ^ ((_P) | ((_D) & (~(_S))))))
#define ROP3_C2(_D, _P, _S) (~((_S) ^ ((_P) | (~((_D) | (_S))))))
#define ROP3_C3(_D, _P, _S) (~((_P) ^ (_S)))
#define ROP3_C4(_D, _P, _S) ((_S) & ((_P) | (~(_D))))
#define ROP3_C5(_D, _P, _S) (~((_S) ^ ((_P) | ((_D) ^ (_S)))))
#define ROP3_C6(_D, _P, _S) ((_S) ^ ((_D) & (~(_P))))
#define ROP3_C7(_D, _P, _S) (~((_P) ^ ((_S) & ((_D) | (_P)))))
#define ROP3_C8(_D, _P, _S) ((_S) & ((_D) | (_P)))
#define ROP3_C9(_D, _P, _S) (~((_S) ^ ((_P) | (_D))))
#define ROP3_CA(_D, _P, _S) ((_D) ^ ((_P) & ((_S) ^ (_D))))
#define ROP3_CB(_D, _P, _S) (~((_S) ^ ((_P) | ((_D) & (_S)))))
#define ROP3_CC(_D, _P, _S) _S
#define ROP3_CD(_D, _P, _S) ((_S) | (~((_D) | (_P))))
#define ROP3_CE(_D, _P, _S) ((_S) | ((_D) & (~(_P))))
#define ROP3_CF(_D, _P, _S) ((_S) | (~(_P)))
#define ROP3_D0(_D, _P, _S) ((_P) & ((_S) | (~(_D))))
#define ROP3_D1(_D, _P, _S) (~((_P) ^ ((_S) | ((_D) ^ (_P)))))
#define ROP3_D2(_D, _P, _S) ((_P) ^ ((_D) & (~(_S))))
#define ROP3_D3(_D, _P, _S) (~((_S) ^ ((_P) & ((_D) | (_S)))))
#define ROP3_D4(_D, _P, _S) ((_S) ^ (((_S) ^ (_P)) & ((_P) ^ (_D))))
#define ROP3_D5(_D, _P, _S) (~((_D) & (~((_P) & (_S)))))
#define ROP3_D6(_D, _P, _S) ((_P) ^ ((_S) ^ ((_D) | ((_P) & (_S)))))
#define ROP3_D7(_D, _P, _S) (~((_D) & ((_P) ^ (_S))))
#define ROP3_D8(_D, _P, _S) ((_P) ^ ((_D) & ((_S) ^ (_P))))
#define ROP3_D9(_D, _P, _S) (~((_S) ^ ((_D) | ((_P) & (_S)))))
#define ROP3_DA(_D, _P, _S) ((_D) ^ ((_P) & (~((_S) & (_D)))))
#define ROP3_DB(_D, _P, _S) (~(((_S) ^ (_P)) & ((_D) ^ (_S))))
#define ROP3_DC(_D, _P, _S) ((_S) | ((_P) & (~(_D))))
#define ROP3_DD(_D, _P, _S) ((_S) | (~(_D)))
#define ROP3_DE(_D, _P, _S) ((_S) | ((_D) ^ (_P)))
#define ROP3_DF(_D, _P, _S) ((_S) | (~((_D) & (_P))))
#define ROP3_E0(_D, _P, _S) ((_P) & ((_D) | (_S)))
#define ROP3_E1(_D, _P, _S) (~((_P) ^ ((_D) | (_S))))
#define ROP3_E2(_D, _P, _S) ((_D) ^ ((_S) & ((_P) ^ (_D))))
#define ROP3_E3(_D, _P, _S) (~((_P) ^ ((_S) | ((_D) & (_P)))))
#define ROP3_E4(_D, _P, _S) ((_S) ^ ((_D) & ((_P) ^ (_S))))
#define ROP3_E5(_D, _P, _S) (~((_P) ^ ((_D) | ((_S) & (_P)))))
#define ROP3_E6(_D, _P, _S) ((_S) ^ ((_D) & (~((_P) & (_S)))))
#define ROP3_E7(_D, _P, _S) (~(((_S) ^ (_P)) & ((_P) ^ (_D))))
#define ROP3_E8(_D, _P, _S) ((_S) ^ (((_S) ^ (_P)) & ((_D) ^ (_S))))
#define ROP3_E9(_D, _P, _S) (~((_D) ^ ((_S) ^ ((_P) & (~((_D) & (_S)))))))
#define ROP3_EA(_D, _P, _S) ((_D) | ((_P) & (_S)))
#define ROP3_EB(_D, _P, _S) ((_D) | (~((_P) ^ (_S))))
#define ROP3_EC(_D, _P, _S) ((_S) | ((_D) & (_P)))
#define ROP3_ED(_D, _P, _S) ((_S) | (~((_D) ^ (_P))))
#define ROP3_EE(_D, _P, _S) ((_D) | (_S))
#define ROP3_EF(_D, _P, _S) ((_S) | ((_D) | (~(_P))))
#define ROP3_F0(_D, _P, _S) _P
#define ROP3_F1(_D, _P, _S) ((_P) | (~((_D) | (_S))))
#define ROP3_F2(_D, _P, _S) ((_P) | ((_D) & (~(_S))))
#define ROP3_F3(_D, _P, _S) ((_P) | (~(_S)))
#define ROP3_F4(_D, _P, _S) ((_P) | ((_S) & (~(_D))))
#define ROP3_F5(_D, _P, _S) ((_P) | (~(_D)))
#define ROP3_F6(_D, _P, _S) ((_P) | ((_D) ^ (_S)))
#define ROP3_F7(_D, _P, _S) ((_P) | (~((_D) & (_S))))
#define ROP3_F8(_D, _P, _S) ((_P) | ((_D) & (_S)))
#define ROP3_F9(_D, _P, _S) ((_P) | (~((_D) ^ (_S))))
#define ROP3_FA(_D, _P, _S) ((_D) | (_P))
#define ROP3_FB(_D, _P, _S) ((_D) | ((_P) | (~(_S))))
#define ROP3_FC(_D, _P, _S) ((_P) | (_S))
#define ROP3_FD(_D, _P, _S) ((_P) | ((_S) | (~(_D))))
#define ROP3_FE(_D, _P, _S) ((_D) | ((_P) | (_S)))
#define ROP3_FF(_D, _P, _S) (~0)

