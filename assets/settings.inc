const size_t START_ITERS_CNT = 64;
const double START_R_CIRCLE_INF = 10;
const double START_SCALE = 550;

Uint32 get_color(size_t iter)
{
  return (Uint32)(
    (iter == START_ITERS_CNT) 
    ? 0xFF000000 
    : (0xFF000000 
      | (MIN(255, iter * 5) << 16) 
      | (MIN(255, iter * 10) << 8) 
      | MIN(255, iter * 20))
  );
}