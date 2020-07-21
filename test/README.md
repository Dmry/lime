## Unit testing

Unit testing is currently mostly limited to validation testing. Generated results will be compared to known (literature) data and the test framework will error when errors exceed 8%. This number was chosen on the basis of the distance between the opposing sides of the curve in the prints. Data was digitized using [WebPlotdigitizer](https://apps.automeris.io/wpd/).

## Running tests

Assuming you have followed the compilation instructions fromt the top readme:

```
make lime_test
./lime_test
```

Specific tests can be chosen by [filtering tags](https://www.boost.org/doc/libs/1_64_0/libs/test/doc/html/boost_test/runtime_config/test_unit_filtering.html). E.g. only running validation tests is done by running:

```
./lime_test --run_test=@validation
```