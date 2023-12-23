

# Simplified C Code for the Navball
```c
void generateNavBall(imageRGB *texture, imageRGB *navballImage, float pitch, float roll, float yaw)
{
    double cs, ss, ct, st, cy, sy;
    float lat, lon;
    uint8_t r,g,b;
    int x, y;
    if (texture->width <= 0 || texture->height <= 0)
    {
        printf("Error texture image size is not valid \n");
        exit(1);
    }
    // create the navball
    // I consider the pitch, roll and yaw are in radian
    // Create two array px,py going from -1 to 1 with a step of 2/size
    // THIS STEP IS DONE IN THE MESHGRID FUNCTION
    // generatePixelArray(px, navballImage->width);
    // generatePixelArray(py, navballImage->height);
    // generatePixelXY(px, py);

    // create the meshgrid hx, hy
    meshgrid(hx, hy);

    // create the meshgrid hz and hit
    // compute_hz(hx, hy, hz, hit);
    compute_hz2(hx, hy, hz);


    cs = cos(roll);
    ss = sin(roll);
    ms[0][0] = cs;
    ms[0][2] = ss;
    ms[2][0] = -ss;
    ms[2][2] = cs;

    ct = cos(pitch);
    st = sin(pitch);
    mt[1][1] = ct;
    mt[1][2] = st;
    mt[2][1] = -st;
    mt[2][2] = ct;

    cy = cos(yaw);
    sy = sin(yaw);
    my[0][0] = cy;
    my[0][1] = sy;
    my[1][0] = -sy;
    my[1][1] = cy;

    // adding pitch
    tensorDot2InPlace(hx, hy, hz, mt);
    // adding roll
    tensorDot2InPlace(hx, hy, hz, ms);
    // adding yaw
    tensorDot2InPlace(hx, hy, hz, my);

    float r2;
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            r2 = hx[i][j] * hx[i][j] + hy[i][j] * hy[i][j];
            if(r2 <= 1.0)
            {
                x = (int)((0.5 + (asin(hy[i][j])) / M_PI) * texture->height);
                y = (int)((1.0 + atan2(hz[i][j], hx[i][j]) / M_PI) * 0.5 * texture->width);
                r = (int) texture->data[x][y][0];
                g = (int) texture->data[x][y][1];
                b = (int) texture->data[x][y][2];
                navballImage->data[i][j][0] = r;
                navballImage->data[i][j][1] = g;
                navballImage->data[i][j][2] = b;
            }
            else
            {
                // set the color to black
                navballImage->data[i][j][0] = 0;
                navballImage->data[i][j][1] = 0;
                navballImage->data[i][j][2] = 0;
            }
        }
    }
}

void tensorDot2InPlace(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], double m[3][3])
{
    float tmp_hx = 0;
    float tmp_hy = 0;
    float tmp_hz = 0;
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            tmp_hx = hx[i][j];
            tmp_hy = hy[i][j];
            tmp_hz = hz[i][j];
            hx[i][j] = tmp_hx * m[0][0] + tmp_hy * m[0][1] + tmp_hz * m[0][2];
            hy[i][j] = tmp_hx * m[1][0] + tmp_hy * m[1][1] + tmp_hz * m[1][2];
            hz[i][j] = tmp_hx * m[2][0] + tmp_hy * m[2][1] + tmp_hz * m[2][2];
        }
    }
}

void compute_hz2(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL])
{
    float r2;
    // compute hz and hit
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            r2 = hx[i][j] * hx[i][j] + hy[i][j] * hy[i][j];
            if (r2 <= 1.0)
            {
                // if hit the hz[i,j] = -np.sqrt(1.0-np.where(hit,r2,0.0)
                hz[i][j] = -sqrt(1.0 - r2); 
            }
            else
            {
                hz[i][j] = NAN;
            }
        }
    }
}

void meshgrid(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL])
{
    float step = 2.0 / (float)SIZE_NAVBALL;
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            // hx[i][j] = px[j];
            hx[i][j] = -1.0 + (float)j * step + 1.0 / (float)SIZE_NAVBALL;
        }
    }
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            // hy[i][j] = py[i];
            hy[i][j] = -1.0 + (float)i * step + 1.0 / (float)SIZE_NAVBALL;
        }
    }
}

```

# mathematical formula
$$
NAVBALL_{size} = 256\\
ms\ matrix =
\begin{bmatrix}
    cos(roll) & 0 & sin(roll) \\
    0 & 1 & 0 \\
    -sin(roll) & 0 & cos(roll)
\end{bmatrix}\\
\newline
mt\ matrix :
\begin{bmatrix}
    1 & 0 & 0 \\
    0 & cos(pitch) & sin(pitch) \\
    0 & -sin(pitch) & cos(pitch)
\end{bmatrix}\\
\newline
my\ matrix :
\begin{bmatrix}
    cos(yaw) & sin(yaw) & 0 \\
    -sin(yaw) & cos(yaw) & 0 \\
    0 & 0 & 1
\end{bmatrix}\\
\newline
\newline
\text{hx, hy, hz are 2D array of size NAVBALL}_{size} \times \text{NAVBALL}_{size}\\
step = \frac{2}{NAVBALL_{size}}\\
i \in [0, NAVBALL_{size}-1]\\
j \in [0, NAVBALL_{size}-1]\\
\quad \quad hx[i][j] = -1.0 + j \times \text{step} + \frac{1.0}{\text{SIZE\_NAVBALL}}\\
\quad \quad hy[i][j] = -1.0 + i \times \text{step} + \frac{1.0}{\text{SIZE\_NAVBALL}}\\
\newline
\text{r2} = hx[i][j] \times hx[i][j] + hy[i][j] \times hy[i][j]\\
\newline
\text{if } r2 \leq 1.0 \text{ then}\\
\quad \quad hz[i][j] = -\sqrt{1.0 - r2}\\
x = ((0.5 + (asin(hy[i][j])) / 	\pi) * height);\\
y = ((1.0 + atan2(hz[i][j], hx[i][j]) / 	\pi) * 0.5 * width);\\
navballImage[i][j] = texture[x][y]\\
\text{else}\\
\quad \quad hz[i][j] = \text{NAN}\\
navballImage[i][j] = 0\\
\newline
\newline
% compute the x, y

\newline
\newline
% put result in image




$$
