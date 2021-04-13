function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function reloadModulesPlot() {
    console.log('reloadModulesPlot() called');

    var but = document.getElementsByName('modulesPlotButton')[0];
    but.style.pointerEvents = 'none';

    var ifr = document.getElementsByName('modulesPlotIframe')[0];
    ifr.src = "/loader-animation";

    var response = await fetch('/loading-modules-plot');
    var loading = await response.json();

    if(loading===true){
        var retries = 0;
        while(loading){
            if(retries > 15)
                break;

            try{
                response = await fetch('/loading-modules-plot');
                loading = await response.json();
                if(loading===true){
                    await sleep(700);
                    retries++;
                }
                else
                    break;
            }
            catch(err){
                console.log(err);
                await sleep(700);
                retries++;
            }
        }

        if(loading===false)
            ifr.src = "/get-modules-plot";
        else
            ifr.src = "/failed-loading-plot";
    }
    else{
        response = await fetch('/load-modules-plot', {method:'POST'});
        ifr.src = "/get-modules-plot";
    }

    but.style.pointerEvents = 'auto';
}

async function viewModulesPlot() {
    console.log('viewModulesPlot() called');

    var but = document.getElementsByName('viewModulesPlotButton')[0];
    but.style.pointerEvents = 'none';

    var ifr = document.getElementsByName('modulesPlotIframe')[0];
    ifr.src = "/loader-animation";

    var response = await fetch('/get-modules-plot');
    response = await response.text();

    if(response.length > 0){
        ifr.src = "/get-modules-plot";
    }
    else
        await reloadModulesPlot();

    but.style.pointerEvents = 'auto';
}

document.addEventListener('DOMContentLoaded', function() {
    var elems = document.querySelectorAll('.sidenav');
    var instances = M.Sidenav.init(elems);
});