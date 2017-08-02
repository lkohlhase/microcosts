void pair(int ref, int test) /* count a beat label pair */
{
    switch (ref)
    {
        case 'N':
            switch (test)
            {
                case 'N':
                    Nn++;
                    break;
                case 'S':
                    Ns++;
                    break;
                case 'V':
                    Nv++;
                    break;
                case 'F':
                    Nf++;
                    break;
                case 'Q':
                    Nq++;
                    break;
                case 'O':
                    No++;
                    break;
                case 'X':
                    Nx++;
                    break;

            }
            break;
    }
}
