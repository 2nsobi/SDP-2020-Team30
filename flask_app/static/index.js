function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function reloadModulesPlot() {
    console.log('reloadModulesPlot() called');

    var but = document.getElementsByName('modulesPlotButton')[0];
    but.style.pointerEvents = 'none';

    fetch('/load-modules-plot', {method:'POST'})
    while(true){
        var response = await fetch('/loading-modules-plot');
        response = await response.json();
        if(response===true)
            await sleep(700);
        else
            break;
    }

    var ifr = document.getElementsByName('modulesPlotIframe')[0];
    ifr.src = "/get-modules-plot";

    but.style.pointerEvents = 'auto';
}

async function viewModulesPlot() {
    console.log('viewModulesPlot() called');

    var but = document.getElementsByName('viewModulesPlotButton')[0];
    but.style.pointerEvents = 'none';

    var response = await fetch('/get-modules-plot');
    response = await response.text();

    if(response.length > 0){
        var ifr = document.getElementsByName('modulesPlotIframe')[0];
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