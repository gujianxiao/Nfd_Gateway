macro MultiLineComment()

{

    hwnd = GetCurrentWnd()

    selection = GetWndSel(hwnd)

    LnFirst =GetWndSelLnFirst(hwnd)      //?????

    LnLast =GetWndSelLnLast(hwnd)      //?????

    hbuf = GetCurrentBuf()

 

    if(GetBufLine(hbuf, 0) =="//magic-number:tph85666031"){

        stop

    }

 

    Ln = Lnfirst

    buf = GetBufLine(hbuf, Ln)

    len = strlen(buf)

 

    while(Ln <= Lnlast) {

        buf = GetBufLine(hbuf, Ln)  //?Ln????

        if(buf ==""){                   //????

            Ln = Ln + 1

            continue

        }

 

        if(StrMid(buf, 0, 1) == "/"){       //??????,?????????

            if(StrMid(buf, 1, 2) == "/"){

                PutBufLine(hbuf, Ln, StrMid(buf, 2, Strlen(buf)))

            }

        }

 

        if(StrMid(buf,0,1) !="/"){          //??????

            PutBufLine(hbuf, Ln, Cat("//", buf))

        }

        Ln = Ln + 1

    }

 

    SetWndSel(hwnd, selection)

}
