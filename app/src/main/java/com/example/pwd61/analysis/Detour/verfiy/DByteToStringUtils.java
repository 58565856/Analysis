package com.example.pwd61.analysis.Detour.verfiy;

/**************************************************************************
 * project:FuckJD
 * Email: 
 * file:DByteToStringUtils
 * Created by pwd61 on 2019/4/4 10:23
 * description:
 *
 *
 *
 *
 *
 ***************************************************************************/


public class DByteToStringUtils
{
    private static final int[] c = new int[]{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    private static final int[] d = new int[]{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    private static final int e = -1;
    private static final int f = -2;

    private int g;
    private int h;
    private final int[] i;

    public static byte[] a;
    public int b;

    public DByteToStringUtils(int arg3, byte[] arg4) {
        this.a = arg4;
        int[] v0 = (arg3 & 8) == 0 ? this.c : this.d;
        this.i = v0;
        this.g = 0;
        this.h = 0;
    }

    public final int a(int arg2) {
        return arg2 * 3 / 4 + 10;
    }

    public final boolean a(byte[] arg10, int arg11, int arg12, boolean arg13) {
        boolean v0;
        if(this.g == 6) {
            v0 = false;
        }
        else {
            int v4 = arg12 + arg11;
            int v2 = this.g;
            int v1 = this.h;
            int v0_1 = 0;
            byte[] v5 = this.a;
            int[] v6 = this.i;
            int v3 = v2;
            v2 = arg11;
            while(v2 < v4) {
                if(v3 == 0) {
                    while(v2 + 4 <= v4) {
                        v1 = v6[arg10[v2] & 255] << 18 | v6[arg10[v2 + 1] & 255] << 12 | v6[arg10[v2 + 2] & 255] << 6 | v6[arg10[v2 + 3] & 255];
                        if(v1 < 0) {
                            break;
                        }

                        v5[v0_1 + 2] = ((byte)v1);
                        v5[v0_1 + 1] = ((byte)(v1 >> 8));
                        v5[v0_1] = ((byte)(v1 >> 16));
                        v0_1 += 3;
                        v2 += 4;
                    }

                    if(v2 >= v4) {
                        break;
                    }
                }

                arg11 = v2 + 1;
                v2 = v6[arg10[v2] & 255];
                switch(v3) {
                    case 0: {
                        if(v2 >= 0) {
                            ++v3;
                            v1 = v2;
                            v2 = arg11;
                            continue;
                        }

                        if(v2 == -1) {
                            v2 = arg11;
                        }

                        this.g = 6;
                        return false;
                    }
                    case 1: {
                        if(v2 >= 0) {
                            v1 = v1 << 6 | v2;
                            ++v3;
                            v2 = arg11;
                            continue;
                        }

                        if(v2 == -1) {
                            v2 = arg11;
                        }

                        this.g = 6;
                        return false;
                    }
                    case 2: {
                        if(v2 >= 0) {
                            v1 = v1 << 6 | v2;
                            ++v3;
                            v2 = arg11;
                            continue;
                        }

                        if(v2 == -2) {
                            v5[v0_1] = ((byte)(v1 >> 4));
                            v3 = 4;
                            ++v0_1;
                            v2 = arg11;
                            continue;
                        }

                        if(v2 == -1) {
                            v2 = arg11;
                        }

                        this.g = 6;
                        return false;
                    }
                    case 3: {
                        if(v2 >= 0) {
                            v1 = v1 << 6 | v2;
                            v5[v0_1 + 2] = ((byte)v1);
                            v5[v0_1 + 1] = ((byte)(v1 >> 8));
                            v5[v0_1] = ((byte)(v1 >> 16));
                            v0_1 += 3;
                            v3 = 0;
                            v2 = arg11;
                            continue;
                        }

                        if(v2 == -2) {
                            v5[v0_1 + 1] = ((byte)(v1 >> 2));
                            v5[v0_1] = ((byte)(v1 >> 10));
                            v0_1 += 2;
                            v3 = 5;
                            v2 = arg11;
                            continue;
                        }

                        if(v2 == -1) {
                            v2 = arg11;
                        }

                        this.g = 6;
                        return false;
                    }
                    case 4: {
                        if(v2 == -2) {
                            ++v3;
                            v2 = arg11;
                            continue;
                        }

                        if(v2 == -1) {
                            v2 = arg11;
                        }

                        this.g = 6;
                        return false;
                    }
                    case 5: {
                        if(v2 == -1) {
                            v2 = arg11;
                        }

                        this.g = 6;
                        return false;
                    }
                }
                v2 = arg11;
            }

            v2 = v1;
            switch(v3) {
                case 1: {
                    this.g = 6;
                    return false;
                }
                case 2: {
                    v5[v0_1] = ((byte)(v2 >> 4));
                    ++v0_1;
                }
                case 3: {
                    v1 = v0_1 + 1;
                    v5[v0_1] = ((byte)(v2 >> 10));
                    v0_1 = v1 + 1;
                    v5[v1] = ((byte)(v2 >> 2));
                    this.g = v3;
                    this.b = v0_1;
                    v0 = true;
                }
                case 4: {
                    this.g = 6;
                    return false;
                }
            }

            this.g = v3;
            this.b = v0_1;
            v0 = true;

            return v0;
        }
        return v0;
    }

}
