# include <cstdio>
# include <cmath>

using namespace std;

int max(int a, int b)
{
    return a > b ? a : b;
}

void adjust(int *line, int line_dim1, int line_dim2, float k, float b, float downscale, int* img, int y_size, int x_size)
// Here I am assuming that the polylines have fewer than 200 points in order to process it in C++ && pass back to python
{
    // line is pre-assigned with a size of 50 * 2, each row [x, y]
    // Note due to swig wrapping, line is now a 1D array. To access line[i][j], use line[i * line_dim1 + j] instead

    // With a good initial guess, adjust the extracted lane lines along the way to the middle of the lane-marking
    // line function: y = kx + b
    int x0, y0;
    if ((k < 0) && (b < y_size - 1))
    {
        x0 = 0;
        y0 = static_cast<int>(b);
    }
    else if ((k > 0) && (k * (x_size - 1) + b < y_size - 1))
    {
        x0 = x_size - 1;
        y0 = static_cast<int>(k * (x_size - 1) + b);
    }
    else
    {
        y0 = y_size - 1;
        x0 = static_cast<int>((y0 - b) / k);
    }
    int x = x0; int y = y0; // x, y starts from intercepts at the bottom of the image

    float dy = - abs(k / sqrt(1 + k*k));
    float dx = dy / (k / sqrt(1 + k*k)) * 1.0 / sqrt(1 + k*k);
    int length = 0, step = 0, all_step = 0;

    while ( ((img[y * y_size + x] == 0) || (step < 2)) &&
        all_step < max(y_size * abs(x - x_size / 2) / (x_size / 2), y_size / 10) )
    {
        length += 1;
        x = x0 + static_cast<int>(length * dx);
        y = y0 + static_cast<int>(length * dy);
        if (img[y * y_size + x] == 255)
            step += 1;
        all_step += 1;
    }

    x0 = x; y0 = y;
    int l = x, r = x;
    while ((img[y * y_size + l] == 255) && l > 0) l -= 1;
    while ((img[y * y_size + r] == 255) && r < x_size - 1) r += 1;
    x = (l + r) / 2;
    length = 0;
    int tot = 0;
    if (img[y * y_size + x] == 255)
    {
        line[tot * 2] = x; // found the first point *on* the lane marking
        line[tot * 2 + 1] = y;
        tot ++;
    }

    // the 0.4, 0.6, 0.4 are define the mask we are applying
    // has to be lower than vanishing point, || the ipm'ed polyline's end point will be negative
    int step_l = 0, step_r = 0;
    while ( (x > 0) && (x < x_size - 1) && \
          (y > 0.05 * y_size || (x > 0.45 * x_size && x < 0.55 * x_size)) ) // ( (x < int(x_size * 0.3)) || (x > int(x_size * 0.7)) || (y > int(0.45 * y_size)) ):
    {
        length += 5 * downscale;
        while (1)
        {
            x = x0 + static_cast<int>(length * dx);
            y = y0 + static_cast<int>(length * dy);
            if (x < 0 || x > x_size - 1 || y < 0 || y > y_size - 1)
                break;
            if (y != y0) // make sure the line is progressing instead of stuck due to small slope
                break;
            else
                length += 5;
        }
        if (x < 0 || x > x_size - 1 || y < 0 || y > y_size - 1)
        {
            length -= 20;
            x = x0 + static_cast<int>(length * dx);
            y = y0 + static_cast<int>(length * dy);
            if (tot * 2 + 1 > line_dim1)
                throw "Error: polyline points number exceeds 50!";
            line[tot * 2] = x; // found the first point *on* the lane marking
            line[tot * 2 + 1] = y;
            tot ++;
            break;
        }

        if (x < x_size - 1 && x > 0 && y > 0 && y < y_size - 1)
            if (img[y * y_size + x] == 255)
            {
                l = x;
                r = x;
                while (img[y * y_size + l] == 255 && l > 0) l -= 1;
                while (img[y * y_size + r] == 255 && r < x_size - 1) r += 1;
                if ( (abs(((l + r) / 2) - x) > (r - l) / 5) && abs(dy) > 0.01 ) // if the deviation is too large from the center of the lane marker
                // if dy < 0.05, the line is too flat. will not adjust x to middle
                {
                    x = (l + r) / 2;
                    if (tot * 2 + 1 > line_dim1)
                        throw "Error: polyline points number exceeds 50!";
                    line[tot * 2] = x; // found the first point *on* the lane marking
                    line[tot * 2 + 1] = y;
                    tot ++;                    
                    x0 = x;
                    y0 = y;
                    length = 0;
                }
                else
                {
                    if (tot * 2 + 1 > line_dim1)
                        throw "Error: polyline points number exceeds 50!";
                    line[tot * 2] = x; // found the first point *on* the lane marking
                    line[tot * 2 + 1] = y;
                    tot ++;
                }
            }
            else
            {
                l = x;
                r = x;
                step_l = 0;
                while (img[y * y_size + l] == 0 && l > 0 && step_l < 10 * downscale)
                {
                    l -= 1;
                    step_l += 1;
                    if (img[y * y_size + l] == 255)
                        break;
                }
                step_r = 0;
                while (img[y * y_size + r] == 0 && r < x_size - 1 && step_r < 10 * downscale)
                {
                    r += 1;
                    step_r += 1;
                    if (img[y * y_size + r] == 255)
                        break;
                }
                if (img[y * y_size + r] == 0 && img[y * y_size + l] == 0) // there is a space here!
                    continue;
                else
                {
                    if (img[y * y_size + r] == 255)
                        l = r;
                    else
                        r = l;
                    while (img[y * y_size + l] == 255 && l > 0) l -= 1;
                    while (img[y * y_size + r] == 255 && r < x_size - 1) r += 1;
                    x = (l + r) / 2;
                    if (tot * 2 + 1 > line_dim1)
                        throw "Error: polyline points number exceeds 50!";
                    line[tot * 2] = x; // found the first point *on* the lane marking
                    line[tot * 2 + 1] = y;
                    tot ++;
                    x0 = x;
                    y0 = y;
                    length = 0;
                }
            }
    }
    
    return;
}

int main(){
    return 0;
}